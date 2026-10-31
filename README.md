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

Para empezar sin escribir reglas, elige **Cargar ejemplo listo para probar**. Incluye `Llueve` y `Si llueve, entonces hay nubes`. Después elige **Consultar verdad de una frase** y prueba `Hay nubes` (verdadera), `No llueve` (falsa) y `Hace frío` (indeterminada). Para ingresar tus propias reglas, elige **Agregar mis propias reglas** y escribe una por línea; deja una línea vacía para terminar. La opción **Ayuda rápida** muestra esta guía en el programa. **Limpiar base de conocimiento** elimina las reglas y los átomos registrados.

Se reconocen estos conectores:

| Texto | Símbolo |
| --- | --- |
| `y` | `∧` |
| `o` | `∨` |
| `no` | `¬` |
| `si ..., entonces ...` | `→` |
| `... si y solo si ...` | `↔` |

Las frases atómicas se identifican por su texto normalizado; repetir una proposición reutiliza su letra. Las letras (`p`, `q`, etc.) son nombres simbólicos de proposiciones, no una codificación *one-hot*. El analizador es sintáctico, no un modelo de comprensión general del español: conviene escribir los conectores con las formas indicadas y usar paréntesis cuando una frase pueda tener más de una lectura.

El tableau supone verdaderas todas las reglas. Aplica las expansiones de los cinco conectores, marca las fórmulas aplicadas, cierra las ramas con un átomo y su negación, y muestra las asignaciones de las ramas abiertas. Si no queda ninguna rama abierta, informa que la base de conocimiento es insatisfacible.

La consulta de verdad compara la base con la frase y con su negación: informa que es verdadera si la base implica la frase, falsa si implica su negación e indeterminada si ambas opciones son compatibles con las reglas. Si las reglas son inconsistentes o se alcanza el límite de expansión, lo indica en vez de afirmar un resultado. Esto determina consecuencias lógicas de la información ingresada; no comprueba por sí solo si un hecho es verdadero en el mundo real.

## Caso de prueba

Introduce estas tres reglas:

```text
Si es de día, entonces voy a trabajar
Si llueve, entonces no voy a trabajar
Es de día
```

El árbol resultante tiene dos ramas cerradas y una abierta, con el modelo `p = V, q = V, r = F`.