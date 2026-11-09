# TPSCombatSandbox

**C++ Gameplay Programming | Unreal Engine 5.7 | Third-Person Combat & Tactical AI**

TPSCombatSandbox is an ongoing third-person combat project built in Unreal Engine 5.7, with an emphasis on C++ gameplay architecture, character and weapon systems, and tactical enemy AI.

The project began as a focused combat prototype and has expanded to include inventory, equipment, and interaction systems. Its core gameplay logic is implemented in C++, with Unreal Engine Blueprints used for animation integration, asset configuration, UI, and editor-exposed properties.

Rather than building a complete game around predetermined mechanics, the project serves as a development environment for implementing, testing, and refining interconnected gameplay systems.

## Gameplay Preview

![TPSCombatSandbox gameplay](https://raw.githubusercontent.com/MrAmmarHaf/github-portfolio/main/projects/tps-combat-sandbox/gameplay-screenshot.png)

*Development screenshot. Character models, environment assets, and visual presentation are subject to change.*

A gameplay demonstration covering combat, enemy perception, cover behavior, and investigation is planned.

## Technical Highlights

### 1. Tactical Enemy AI

The enemy AI is implemented through a custom C++ state machine built around `AEnemyCharacter`, with movement completion handled by `AEnemyAIController`.

The system coordinates enemy perception, combat decisions, movement, and state transitions.

**Key implementations:**

- Field-of-view detection using vector dot products
- Line-of-sight checks using collision traces
- Patrol routes with configurable waiting intervals
- Target tracking and last-known-position memory
- Navigation-based chasing and repositioning
- Burst-fire combat with configurable timing
- Cover detection and validation
- Investigation and area searching after losing the player
- Suppression responses and damage handling
- Configurable pistol, automatic-rifle, and shotgun enemy behaviors

**Cover evaluation**

Enemies search for nearby obstacles through radial collision traces. Candidate positions are projected onto the navigation mesh and evaluated against the player's position.

The cover validation checks whether the expected obstacle blocks the player's view toward the candidate position, including multiple points across the approximate width of the enemy.

When a position is selected, the AI can navigate into cover and transition into a timed peek-and-fire sequence.

**Investigation and searching**

When an enemy loses sight of the player, it retains the last known location rather than immediately returning to an idle state.

Depending on the current combat state, the AI can delay its investigation, move toward the remembered position, search nearby reachable areas, or resume patrolling.

These behaviors are coordinated through timers, state checks, and Unreal Engine's navigation system.

### 2. Player and Weapon Systems

Player gameplay is implemented primarily through `ACombatCharacter`, with weapon-specific properties and ammunition behavior handled by `ACombatWeapon`.

**Implemented mechanics include:**

- Walking, running, sprinting, crouching, and jumping
- Aim-down-sights camera transitions
- Directional leaning while aiming
- Pistol and rifle equipment
- Primary and secondary weapon switching
- Weapon pickup, holstering, and unholstering
- Semi-automatic and automatic firing
- Camera-based line-trace hit detection
- Bone-based headshot damage
- Ammunition management and reloading
- Hit reactions, health, and death handling

**Action coordination**

Weapon interactions use an `EWeaponActionState` enumeration to represent operations such as picking up, switching, holstering, unholstering, and reloading.

Gameplay functions check the current action state before beginning another operation, helping prevent conflicting actions such as firing during a reload or starting multiple weapon transitions simultaneously.

Animation Montages and callback functions are used to synchronize visual actions with gameplay events.

### 3. Inventory and Equipment

The project also contains an evolving inventory system built around `UInventoryComponent` and `UItemDefinition`.

Item definitions use Unreal Engine Primary Data Assets, allowing item properties to be configured through the editor rather than hardcoded for every pickup.

The implementation includes:

- Stackable and non-stackable item definitions
- Quantity and stack-size validation
- Backpack-based item storage
- Pickup interactions
- Head and torso equipment slots
- Equipment meshes attached to character sockets
- Damage and maximum-health modifiers
- Bandage-based healing with animation integration

The inventory system is still being refactored. An earlier item-count implementation remains in use for bandages while the newer data-asset-based system is being developed.

## Architecture

The code separates responsibilities across gameplay actors, components, and data definitions.

| Class / Component | Primary responsibility |
|---|---|
| `ACombatCharacter` | Player input, movement, camera, combat actions, equipment, and interaction |
| `ACombatWeapon` | Weapon configuration, ammunition, pickup collision, and firing animation |
| `AEnemyCharacter` | AI state management, perception, combat decisions, navigation, cover, and damage |
| `AEnemyAIController` | Navigation movement completion and related callbacks |
| `UInventoryComponent` | Item storage, quantities, stacking, and removal |
| `UItemDefinition` | Data-driven item properties, equipment slots, meshes, and modifiers |
| `ABackpack` | Backpack actor and inventory ownership |
| `AItemPickup` | World item representation and item-definition reference |

The main gameplay behaviors are implemented in C++, while `UPROPERTY` and `UFUNCTION` expose selected data and functions to Unreal Engine's editor and Blueprint systems.

This allows core logic to remain in source code while supporting per-character and per-weapon configuration.

## Engineering Decisions

**Custom state machine for AI**

A C++ state machine was chosen to provide direct control over combat transitions, timer-driven decisions, and navigation behavior. This also makes the underlying decision logic accessible for debugging and review.

**Separating weapon data from player behavior**

`ACombatWeapon` stores weapon-specific properties such as damage, fire delay, ammunition, and animation references. `ACombatCharacter` coordinates player input, targeting, weapon actions, and damage application.

**Data-driven item definitions**

The newer inventory implementation uses `UPrimaryDataAsset` to define items separately from inventory storage. This supports configurable item categories, stacking rules, equipment properties, and stat modifiers.

**Animation and gameplay synchronization**

Animation Montages and Blueprint-callable completion functions coordinate actions such as reloading and weapon switching, rather than relying exclusively on fixed delays.

## Technology Stack

- **Engine:** Unreal Engine 5.7
- **Programming:** C++
- **Gameplay:** Enhanced Input, collision traces, timers, actor components
- **AI:** AIController, Navigation System, custom state machine
- **Animation:** Animation Blueprints, Blend Spaces, Montages, Animation Notifies
- **UI:** Unreal Motion Graphics (UMG)
- **Development:** Visual Studio, Git, Git LFS

## Development Status

**Active development — playable prototype, not a finished game.**

The project is developed and tested iteratively in the Unreal Editor. Current development focuses on integrating gameplay systems, refining enemy behavior, and expanding player interaction and inventory functionality.

Known areas for improvement include:

- Further separation of AI decision-making from the enemy character class
- More consistent handling of interrupted or failed AI actions
- Continued migration to the data-driven inventory system
- Additional validation and regression testing
- Expanded debugging tools and gameplay demonstrations
- Animation and presentation polish

The project is currently focused on single-player functionality. Multiplayer support is not part of the demonstrated implementation.

## Building and Running

**Requirements**

- Windows
- Unreal Engine 5.7
- Visual Studio with C++ development tools
- Windows SDK
- Git and Git LFS, where applicable

The project uses Unreal Engine modules including `EnhancedInput`, `AIModule`, `NavigationSystem`, `Niagara`, and `UMG`.

Build and runtime setup depend on which assets are included in the published repository. Some Blueprint and animation content used in the development project is licensed from third parties and may not be redistributable.

If this repository contains only the original C++ source and documentation, it is intended for **source inspection rather than direct execution as a complete game**.

## Project Scope and Attribution

TPSCombatSandbox is an independently developed gameplay programming project.

The custom gameplay logic is written in C++, using Unreal Engine's existing framework, animation tools, and navigation systems.

Visual and audio assets used during development may originate from Epic Games, Fab marketplace packages, or other third-party providers and remain subject to their respective licenses.

Only files authorized for redistribution should be included in a public release.
