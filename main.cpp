#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class Kind { Atom, Not, And, Or, Implies, Iff };

struct Formula {
    Kind kind;
    std::string atom;
    std::shared_ptr<const Formula> left;
    std::shared_ptr<const Formula> right;
};

using FormulaPtr = std::shared_ptr<const Formula>;

FormulaPtr atomFormula(const std::string& name) {
    return std::make_shared<Formula>(Formula{Kind::Atom, name, nullptr, nullptr});
}

FormulaPtr unaryFormula(Kind kind, FormulaPtr operand) {
    return std::make_shared<Formula>(Formula{kind, "", std::move(operand), nullptr});
}

FormulaPtr binaryFormula(Kind kind, FormulaPtr left, FormulaPtr right) {
    return std::make_shared<Formula>(Formula{kind, "", std::move(left), std::move(right)});
}

std::string normalize(std::string text) {
    const std::vector<std::pair<std::string, std::string>> accents = {
        {"Á", "á"}, {"É", "é"}, {"Í", "í"}, {"Ó", "ó"}, {"Ú", "ú"}, {"Ü", "ü"}, {"Ñ", "ñ"}};
    for (const auto& accent : accents) {
        std::size_t position = 0;
        while ((position = text.find(accent.first, position)) != std::string::npos) {
            text.replace(position, accent.first.size(), accent.second);
            position += accent.second.size();
        }
    }

    std::string cleaned;
    for (unsigned char character : text) {
        if (std::ispunct(character) && character != '(' && character != ')') {
            cleaned.push_back(' ');
        } else {
            cleaned.push_back(static_cast<char>(std::tolower(character)));
        }
    }

    std::string result;
    bool previousSpace = true;
    for (char character : cleaned) {
        if (std::isspace(static_cast<unsigned char>(character))) {
            if (!previousSpace) result.push_back(' ');
            previousSpace = true;
        } else {
            result.push_back(character);
            previousSpace = false;
        }
    }
    if (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(' ');
    if (first == std::string::npos) return "";
    const auto last = text.find_last_not_of(' ');
    return text.substr(first, last - first + 1);
}

bool hasOuterParentheses(const std::string& text) {
    if (text.size() < 2 || text.front() != '(' || text.back() != ')') return false;
    int depth = 0;
    for (std::size_t index = 0; index < text.size(); ++index) {
        if (text[index] == '(') ++depth;
        if (text[index] == ')') --depth;
        if (depth == 0 && index + 1 != text.size()) return false;
    }
    return depth == 0;
}

std::size_t findTopLevel(const std::string& text, const std::string& delimiter) {
    int depth = 0;
    for (std::size_t index = 0; index + delimiter.size() <= text.size(); ++index) {
        if (text[index] == '(') {
            ++depth;
            continue;
        }
        if (text[index] == ')') {
            --depth;
            continue;
        }
        if (depth == 0 && text.compare(index, delimiter.size(), delimiter) == 0) return index;
    }
    return std::string::npos;
}

class AtomRegistry {
public:
    std::string registerAtom(const std::string& phrase) {
        const auto key = normalize(phrase);
        if (key.empty()) throw std::runtime_error("Se encontró una proposición vacía.");
        const auto found = letters_.find(key);
        if (found != letters_.end()) return found->second;

        const auto letter = letterFor(atoms_.size());
        atoms_.push_back({letter, key});
        letters_[key] = letter;
        return letter;
    }

    const std::vector<std::pair<std::string, std::string>>& atoms() const { return atoms_; }

private:
    static std::string letterFor(std::size_t index) {
        if (index < 11) return std::string(1, static_cast<char>('p' + index));
        const auto offset = index - 11;
        return std::string(1, static_cast<char>('a' + offset % 26)) + std::to_string(offset / 26 + 1);
    }

    std::vector<std::pair<std::string, std::string>> atoms_;
    std::map<std::string, std::string> letters_;
};

class SpanishParser {
public:
    explicit SpanishParser(AtomRegistry& registry) : registry_(registry) {}

    FormulaPtr parse(const std::string& input) {
        return parseNormalized(normalize(input));
    }

private:
    FormulaPtr parseNormalized(std::string text) {
        text = trim(std::move(text));
        while (hasOuterParentheses(text)) text = trim(text.substr(1, text.size() - 2));
        if (text.empty()) throw std::runtime_error("La regla está vacía.");

        auto position = findTopLevel(text, " si y solo si ");
        if (position != std::string::npos) {
            auto left = parseNormalized(text.substr(0, position));
            auto right = parseNormalized(text.substr(position + 14));
            return binaryFormula(Kind::Iff, std::move(left), std::move(right));
        }

        if (text.rfind("si ", 0) == 0) {
            position = findTopLevel(text, " entonces ");
            if (position != std::string::npos) {
                auto left = parseNormalized(text.substr(3, position - 3));
                auto right = parseNormalized(text.substr(position + 10));
                return binaryFormula(Kind::Implies, std::move(left), std::move(right));
            }
        }

        position = findTopLevel(text, " o ");
        if (position != std::string::npos) {
            auto left = parseNormalized(text.substr(0, position));
            auto right = parseNormalized(text.substr(position + 3));
            return binaryFormula(Kind::Or, std::move(left), std::move(right));
        }

        position = findTopLevel(text, " y ");
        if (position != std::string::npos) {
            auto left = parseNormalized(text.substr(0, position));
            auto right = parseNormalized(text.substr(position + 3));
            return binaryFormula(Kind::And, std::move(left), std::move(right));
        }

        if (text.rfind("no ", 0) == 0) {
            return unaryFormula(Kind::Not, parseNormalized(text.substr(3)));
        }
        if (text.compare(0, std::string("¬").size(), "¬") == 0) {
            return unaryFormula(Kind::Not, parseNormalized(text.substr(std::string("¬").size())));
        }

        int depth = 0;
        for (char character : text) {
            if (character == '(') ++depth;
            if (character == ')') --depth;
            if (depth < 0) throw std::runtime_error("Hay paréntesis desbalanceados.");
        }
        if (depth != 0) throw std::runtime_error("Hay paréntesis desbalanceados.");
        return atomFormula(registry_.registerAtom(text));
    }

    AtomRegistry& registry_;
};

std::string formulaText(const FormulaPtr& formula) {
    switch (formula->kind) {
        case Kind::Atom: return formula->atom;
        case Kind::Not: return "¬" + formulaText(formula->left);
        case Kind::And: return "(" + formulaText(formula->left) + " ∧ " + formulaText(formula->right) + ")";
        case Kind::Or: return "(" + formulaText(formula->left) + " ∨ " + formulaText(formula->right) + ")";
        case Kind::Implies: return "(" + formulaText(formula->left) + " → " + formulaText(formula->right) + ")";
        case Kind::Iff: return "(" + formulaText(formula->left) + " ↔ " + formulaText(formula->right) + ")";
    }
    return "";
}

struct Rule {
    std::string text;
    FormulaPtr formula;
};

struct Item {
    FormulaPtr formula;
    bool applied = false;
};

struct TreeNode {
    std::string label;
    std::vector<TreeNode> children;
};

bool isLiteral(const FormulaPtr& formula) {
    return formula->kind == Kind::Atom ||
        (formula->kind == Kind::Not && formula->left->kind == Kind::Atom);
}

bool isNegated(const FormulaPtr& formula) {
    return formula->kind == Kind::Not;
}

FormulaPtr negate(FormulaPtr formula) {
    return unaryFormula(Kind::Not, std::move(formula));
}

std::string literalName(const FormulaPtr& formula) {
    return formula->kind == Kind::Atom ? formula->atom : formula->left->atom;
}

bool findContradiction(const std::vector<Item>& items, std::string& reason) {
    std::map<std::string, int> values;
    for (const auto& item : items) {
        if (!isLiteral(item.formula)) continue;
        const auto name = literalName(item.formula);
        const int value = isNegated(item.formula) ? -1 : 1;
        const auto found = values.find(name);
        if (found != values.end() && found->second != value) {
            reason = name + " y ¬" + name;
            return true;
        }
        values[name] = value;
    }
    return false;
}

std::vector<std::vector<FormulaPtr>> expansions(const FormulaPtr& formula, bool& branching) {
    branching = false;
    const auto left = formula->left;
    const auto right = formula->right;
    switch (formula->kind) {
        case Kind::And: return {{left, right}};
        case Kind::Not:
            switch (left->kind) {
                case Kind::Not: return {{left->left}};
                case Kind::Or: return {{negate(left->left), negate(left->right)}};
                case Kind::Implies: return {{left->left, negate(left->right)}};
                case Kind::And:
                    branching = true;
                    return {{negate(left->left)}, {negate(left->right)}};
                case Kind::Iff:
                    branching = true;
                    return {{left->left, negate(left->right)}, {negate(left->left), left->right}};
                case Kind::Atom: break;
            }
            break;
        case Kind::Or:
            branching = true;
            return {{left}, {right}};
        case Kind::Implies:
            branching = true;
            return {{negate(left)}, {right}};
        case Kind::Iff:
            branching = true;
            return {{left, right}, {negate(left), negate(right)}};
        case Kind::Atom: break;
    }
    return {};
}

std::string branchDescription(const std::vector<FormulaPtr>& formulas) {
    std::string result;
    for (std::size_t index = 0; index < formulas.size(); ++index) {
        if (index != 0) result += ", ";
        result += formulaText(formulas[index]);
    }
    return result;
}

std::string assignmentText(const std::vector<Item>& items,
    const std::vector<std::pair<std::string, std::string>>& atoms) {
    std::map<std::string, bool> values;
    for (const auto& atom : atoms) values[atom.first] = true;
    for (const auto& item : items) {
        if (isLiteral(item.formula)) values[literalName(item.formula)] = !isNegated(item.formula);
    }

    std::string result;
    for (const auto& atom : atoms) {
        if (!result.empty()) result += ", ";
        result += atom.first + " = " + (values[atom.first] ? "V" : "F");
    }
    return result;
}

TreeNode buildTree(std::vector<Item> items,
    const std::vector<std::pair<std::string, std::string>>& atoms, std::size_t& budget) {
    if (budget == 0) return {"Límite de expansión alcanzado", {}};
    --budget;

    std::string contradiction;
    if (findContradiction(items, contradiction)) return {"✕ CERRADA (" + contradiction + ")", {}};

    std::size_t selected = items.size();
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (!items[index].applied && !isLiteral(items[index].formula)) {
            selected = index;
            break;
        }
    }
    if (selected == items.size()) {
        return {"ABIERTA: " + assignmentText(items, atoms), {}};
    }

    const auto current = items[selected].formula;
    bool branching = false;
    const auto options = expansions(current, branching);
    items[selected].applied = true;
    const std::string tag = branching ? "β" : "α";
    TreeNode node{"Aplicar " + formulaText(current) + " [" + tag + ", aplicada]", {}};
    for (std::size_t option = 0; option < options.size(); ++option) {
        auto nextItems = items;
        for (const auto& formula : options[option]) nextItems.push_back({formula, false});
        auto child = buildTree(std::move(nextItems), atoms, budget);
        if (branching) {
            node.children.push_back({"Rama " + std::to_string(option + 1) + ": " +
                branchDescription(options[option]), {std::move(child)}});
        } else {
            node.children.push_back(std::move(child));
        }
    }
    return node;
}

enum class Satisfiability { Satisfiable, Unsatisfiable, LimitReached };

Satisfiability checkSatisfiability(const std::vector<Item>& items,
    const std::vector<std::pair<std::string, std::string>>& atoms) {
    std::size_t budget = 50000;
    const auto tree = buildTree(items, atoms, budget);
    std::vector<const TreeNode*> pending{&tree};
    bool hasOpenBranch = false;
    bool limitReached = false;
    while (!pending.empty()) {
        const auto* node = pending.back();
        pending.pop_back();
        if (node->children.empty()) {
            if (node->label.rfind("ABIERTA:", 0) == 0) hasOpenBranch = true;
            if (node->label == "Límite de expansión alcanzado") limitReached = true;
        } else {
            for (const auto& child : node->children) pending.push_back(&child);
        }
    }

    if (limitReached) return Satisfiability::LimitReached;
    return hasOpenBranch ? Satisfiability::Satisfiable : Satisfiability::Unsatisfiable;
}

void printTree(const TreeNode& node, const std::string& prefix = "", bool last = true, bool root = true) {
    if (root) {
        std::cout << node.label << '\n';
    } else {
        std::cout << prefix << (last ? "`-- " : "|-- ") << node.label << '\n';
    }
    const std::string childPrefix = root ? "" : prefix + (last ? "    " : "|   ");
    for (std::size_t index = 0; index < node.children.size(); ++index) {
        printTree(node.children[index], childPrefix, index + 1 == node.children.size(), false);
    }
}

void addRules(std::vector<Rule>& rules, AtomRegistry& atoms) {
    std::cout << "Escribe una regla por línea; deja una línea vacía para terminar.\n";
    while (true) {
        std::cout << "Regla: ";
        std::string text;
        std::getline(std::cin, text);
        if (text.empty()) break;
        try {
            SpanishParser parser(atoms);
            auto formula = parser.parse(text);
            rules.push_back({text, std::move(formula)});
            std::cout << "Agregada como R" << rules.size() << ": " << formulaText(rules.back().formula) << '\n';
        } catch (const std::exception& error) {
            std::cout << "No se pudo interpretar: " << error.what() << '\n';
        }
    }
}

void loadExample(std::vector<Rule>& rules, AtomRegistry& atoms) {
    if (!rules.empty()) {
        std::cout << "Primero limpia la base de conocimiento (opción 8) para cargar el ejemplo sin mezclarlo.\n";
        return;
    }

    const std::vector<std::string> exampleRules = {
        "Llueve",
        "Si llueve, entonces hay nubes"
    };
    std::vector<Rule> parsedRules;
    try {
        SpanishParser parser(atoms);
        for (const auto& text : exampleRules) {
            parsedRules.push_back({text, parser.parse(text)});
        }
    } catch (const std::exception& error) {
        std::cout << "No se pudo cargar el ejemplo: " << error.what() << '\n';
        return;
    }

    rules.insert(rules.end(), parsedRules.begin(), parsedRules.end());
    std::cout << "Ejemplo cargado. Reglas guardadas:\n";
    for (std::size_t index = 0; index < rules.size(); ++index) {
        std::cout << "R" << index + 1 << ": " << rules[index].text << "  =>  "
                  << formulaText(rules[index].formula) << '\n';
    }
    std::cout << "Ahora elige 7 y prueba: \"Hay nubes\" (verdadera), "
              << "\"No llueve\" (falsa) o \"Hace frío\" (indeterminada).\n";
}

void showRules(const std::vector<Rule>& rules) {
    if (rules.empty()) {
        std::cout << "No hay reglas en la base de conocimiento.\n";
        return;
    }
    for (std::size_t index = 0; index < rules.size(); ++index) {
        std::cout << "R" << index + 1 << ": " << rules[index].text << "  =>  "
                  << formulaText(rules[index].formula) << '\n';
    }
}

void showAtoms(const AtomRegistry& atoms) {
    if (atoms.atoms().empty()) {
        std::cout << "Todavía no hay átomos.\n";
        return;
    }
    std::cout << "Átomos registrados:\n";
    for (const auto& atom : atoms.atoms()) std::cout << "  " << atom.first << " = " << atom.second << '\n';
}

void searchAtoms(const AtomRegistry& atoms) {
    std::cout << "Texto que quieres buscar: ";
    std::string query;
    std::getline(std::cin, query);
    query = normalize(query);
    bool found = false;
    for (const auto& atom : atoms.atoms()) {
        if (atom.second.find(query) != std::string::npos) {
            std::cout << "  " << atom.first << " = " << atom.second << '\n';
            found = true;
        }
    }
    if (!found) std::cout << "No se encontraron átomos coincidentes.\n";
}

void queryTruth(const std::vector<Rule>& rules, AtomRegistry& atoms) {
    std::cout << "Frase que quieres evaluar: ";
    std::string text;
    std::getline(std::cin, text);

    try {
        SpanishParser parser(atoms);
        const auto query = parser.parse(text);
        std::vector<Item> knowledgeBase;
        for (const auto& rule : rules) knowledgeBase.push_back({rule.formula, false});

        const auto baseStatus = checkSatisfiability(knowledgeBase, atoms.atoms());
        if (baseStatus == Satisfiability::Unsatisfiable) {
            std::cout << "La base de conocimiento es inconsistente; no se puede determinar la frase.\n";
            return;
        }
        if (baseStatus == Satisfiability::LimitReached) {
            std::cout << "No se pudo determinar: se alcanzó el límite de expansión.\n";
            return;
        }

        auto withQuery = knowledgeBase;
        withQuery.push_back({query, false});
        auto withNegatedQuery = knowledgeBase;
        withNegatedQuery.push_back({negate(query), false});

        const auto trueStatus = checkSatisfiability(withQuery, atoms.atoms());
        const auto falseStatus = checkSatisfiability(withNegatedQuery, atoms.atoms());
        if (trueStatus == Satisfiability::LimitReached || falseStatus == Satisfiability::LimitReached) {
            std::cout << "No se pudo determinar: se alcanzó el límite de expansión.\n";
        } else if (falseStatus == Satisfiability::Unsatisfiable) {
            std::cout << "\"" << text << "\" es verdadera según las reglas ("
                      << formulaText(query) << ").\n";
        } else if (trueStatus == Satisfiability::Unsatisfiable) {
            std::cout << "\"" << text << "\" es falsa según las reglas ("
                      << formulaText(query) << ").\n";
        } else {
            std::cout << "No se puede determinar si \"" << text
                      << "\" es verdadera o falsa con las reglas actuales ("
                      << formulaText(query) << ").\n";
        }
    } catch (const std::exception& error) {
        std::cout << "No se pudo interpretar: " << error.what() << '\n';
    }
}

void analyze(const std::vector<Rule>& rules, const AtomRegistry& atoms) {
    if (rules.empty()) {
        std::cout << "Agrega al menos una regla antes de construir el árbol.\n";
        return;
    }
    std::cout << "Árbol semántico (se supone que todas las reglas son verdaderas):\n";
    std::string initial = "Inicio: ";
    std::vector<Item> items;
    for (std::size_t index = 0; index < rules.size(); ++index) {
        if (index != 0) initial += ", ";
        initial += "R" + std::to_string(index + 1) + " " + formulaText(rules[index].formula);
        items.push_back({rules[index].formula, false});
    }
    std::size_t budget = 50000;
    auto tree = TreeNode{initial, {buildTree(std::move(items), atoms.atoms(), budget)}};
    printTree(tree);

    std::size_t openBranches = 0;
    std::size_t closedBranches = 0;
    std::vector<const TreeNode*> pending{&tree};
    while (!pending.empty()) {
        const auto* node = pending.back();
        pending.pop_back();
        if (node->children.empty()) {
            if (node->label.find("ABIERTA:") != std::string::npos) ++openBranches;
            if (node->label.find("CERRADA") != std::string::npos) ++closedBranches;
        } else {
            for (const auto& child : node->children) pending.push_back(&child);
        }
    }
    std::cout << "\nResumen: " << closedBranches << " rama(s) cerrada(s), "
              << openBranches << " rama(s) abierta(s).\n";
    if (openBranches == 0) {
        std::cout << "La base de conocimiento es insatisfacible.\n";
    } else {
        std::cout << "La base de conocimiento es satisfacible.\n";
    }
}

int main() {
    AtomRegistry atoms;
    std::vector<Rule> rules;
    while (true) {
        std::cout << "\n=== Árbol semántico ===\n"
                  << "1. Cargar ejemplo listo para probar\n"
                  << "2. Agregar mis propias reglas\n"
                  << "3. Ver reglas y fórmulas guardadas\n"
                  << "4. Ver tabla de átomos\n"
                  << "5. Buscar átomo\n"
                  << "6. Construir árbol semántico\n"
                  << "7. Consultar verdad de una frase\n"
                  << "8. Limpiar base de conocimiento\n"
                  << "9. Ayuda rápida\n"
                  << "0. Salir\n"
                  << "Opción: ";
        std::string option;
        if (!std::getline(std::cin, option)) break;
        if (option == "0") break;
        if (option == "1") {
            loadExample(rules, atoms);
        } else if (option == "2") {
            addRules(rules, atoms);
        } else if (option == "3") {
            showRules(rules);
        } else if (option == "4") {
            showAtoms(atoms);
        } else if (option == "5") {
            searchAtoms(atoms);
        } else if (option == "6") {
            analyze(rules, atoms);
        } else if (option == "7") {
            queryTruth(rules, atoms);
        } else if (option == "8") {
            rules.clear();
            atoms = AtomRegistry{};
            std::cout << "Base de conocimiento vacía.\n";
        } else if (option == "9") {
            std::cout << "Para empezar: elige 1 para cargar el ejemplo y luego 7 para consultar una frase.\n"
                      << "Escribe la frase completa; usa no, y, o, si ... entonces ... o si y solo si.\n"
                      << "Prueba \"Hay nubes\", \"No llueve\" y \"Hace frío\".\n"
                      << "Las consultas se comparan con las reglas guardadas; no verifican hechos del mundo real.\n";
        } else {
            std::cout << "Opción no reconocida.\n";
        }
    }
    return 0;
}