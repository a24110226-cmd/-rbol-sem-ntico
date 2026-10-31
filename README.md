# Árbol semántico

Aplicación de consola en C++17 para convertir reglas en español a fórmulas de lógica proposicional y construir un árbol semántico (tableau).

## Compilar

Requiere un compilador C++17 y CMake.

```sh
cmake -S . -B build
cmake --build build
./build/arbol_semantico
```

## Uso

En el menú, elige **Agregar reglas** e introduce una regla por línea. Deja una línea vacía para terminar. Después puedes consultar las fórmulas, ver la tabla de átomos, buscar por parte del significado y construir el árbol. **Limpiar base de conocimiento** elimina las reglas y los átomos registrados.

Se reconocen estos conectores:

| Texto | Símbolo |
| --- | --- |
| `y` | `∧` |
| `o` | `∨` |
| `no` | `¬` |
| `si ..., entonces ...` | `→` |
| `... si y solo si ...` | `↔` |

Las frases atómicas se identifican por su texto normalizado; repetir una proposición reutiliza su letra. El analizador es sintáctico, no un modelo de comprensión general del español: conviene escribir los conectores con las formas indicadas y usar paréntesis cuando una frase pueda tener más de una lectura.

El tableau supone verdaderas todas las reglas. Aplica las expansiones de los cinco conectores, marca las fórmulas aplicadas, cierra las ramas con un átomo y su negación, y muestra las asignaciones de las ramas abiertas. Si no queda ninguna rama abierta, informa que la base de conocimiento es insatisfacible.

## Caso de prueba

Introduce estas tres reglas:

```text
Si es de día, entonces voy a trabajar
Si llueve, entonces no voy a trabajar
Es de día
```

El árbol resultante tiene dos ramas cerradas y una abierta, con el modelo `p = V, q = V, r = F`.