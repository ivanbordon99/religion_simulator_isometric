# Project Coding Guidelines & Standards / Guía y Estándares de Código

## 1. Variable Naming (Variables siempre en Inglés)
* **Always use English for variables**: All local variables, member variables, parameters, constants, and function/method names created must be in English (never in Spanish).
* **No cryptic or single-letter names**: Avoid meaningless single-letter names (e.g., avoid `x`, `y`, `h`, `d`, `act`, `res`, `c`).
* **Concise but descriptive**: Use short, clear, and meaningful words that describe their intent and context:
  * Instead of `x`, `y` -> use `cellX`, `cellY`, `screenPosX`, `screenPosY`, `gridX`, `gridY`.
  * Instead of `act` -> use `currentAction`, `selectedAction`, `actionType`.
  * Instead of `h` or `hu` -> use `villager`, `selectedVillager`, `humanUnit`.
  * Instead of `d` -> use `distance`, `direction`, `deltaValue`.
  * Instead of `c` -> use `targetCell`, `currentCell`.

## 2. Bilingual Block & Logic Comments (Comentarios Bilingües: Inglés y Español)
* **All comments must be in both English and Spanish**:
  * Example: `// EN: Check house capacity and current sleepers / ES: Verificar la capacidad del hogar y los ocupantes durmiendo`
* **Regular block commentary**: Divide logical workflows into readable functional blocks. Precede significant sections, loops, branches, and algorithmic decisions with bilingual comments detailing:
  1. Purpose & intent (qué hace el bloque).
  2. Core mechanism (cómo lo resuelve).
  3. Preconditions or edge cases handled (condiciones o casos límite).

## 3. Modern Programming Practices (Programación Moderna C++20)
* **Modern C++20 Standards**:
  * Prioritize `constexpr`, `enum class`, and strong typing.
  * Use `std::clamp`, `<algorithm>` primitives (`std::ranges`, `std::find_if`, `std::all_of`) instead of brittle raw index loops where clarity is improved.
  * Const-correctness: Mark methods, parameters, and variables `const` whenever they are not mutated.
  * RAII and memory safety: Avoid manual unmanaged pointers; use references, `std::unique_ptr`/`std::shared_ptr`, or value semantics.
* **No Magic Numbers**: Replace raw numeric literals with descriptive named constants (`constexpr float`, `constexpr int`, or strongly-typed enums).
* **Structure & Legibility**: Single Responsibility Principle for helper routines; keep functions focused and modular.
* **Documentation & Comment Integrity**: Preserve existing comments and documentation integrity.
