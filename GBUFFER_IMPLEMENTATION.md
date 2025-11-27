# G-Buffer Deferred Rendering Implementation

## Changements effectués

### 1. Nouveaux shaders créés

- **`shaders/gbuffer.vert`** : Shader vertex pour le G-Buffer (identique à `basic.vert`)
- **`shaders/gbuffer.frag`** : Shader fragment qui écrit dans 4 render targets :
  - `out_position` (RGBA16F) : Position monde
  - `out_normal` (RGBA16F) : Normale monde (après normal mapping)
  - `out_albedo` (RGBA8) : Couleur de base + alpha
  - `out_metal_rough` (RGBA8) : Metallic (R) + Roughness (G)

- **`shaders/gbuffer_debug.frag`** : Shader de visualisation qui permet d'afficher les différents buffers selon un mode de débogage

### 2. Modifications de RendererState (src/main.cpp)

Ajout des ressources G-Buffer :
```cpp
// G-Buffer textures
Texture gbuffer_position;      // RGBA16F - position monde
Texture gbuffer_normal;        // RGBA16F - normale monde
Texture gbuffer_albedo;        // RGBA8 - couleur de base
Texture gbuffer_metal_rough;   // RGBA8 - metallic + roughness

// Framebuffers
Framebuffer gbuffer_framebuffer;        // Pour écrire le G-Buffer (MRT avec 4 color attachments)
Framebuffer gbuffer_debug_framebuffer;  // Pour la visualisation debug
Texture gbuffer_debug_output;           // Texture de sortie du debug
std::shared_ptr<Program> gbuffer_debug_program;
```

### 3. Modifications du système Material

- Ajout de `Material::gbuffer_material()` dans `src/Material.h` et `src/Material.cpp`
- Cette méthode crée un matériau utilisant les shaders `gbuffer.frag` et `gbuffer.vert`
- Support de l'alpha test (ALPHA_TEST define)

### 4. Modifications du Scene_loader (src/Scene_loader.cpp)

- Tous les matériaux chargés depuis les fichiers glTF utilisent maintenant `Material::gbuffer_material()` au lieu de `Material::textured_pbr_material()`

### 5. Modifications de la boucle de rendu (src/main.cpp)

**Nouvelle architecture :**

1. **Z-prepass** : Rendu de la profondeur dans le G-Buffer framebuffer (optimisation)
2. **Shadow pass** : Génération de la shadow map (conservé)
3. **G-Buffer pass** : Rendu de la scène dans le G-Buffer (4 render targets MRT)
4. **G-Buffer Debug** : Visualisation des buffers dans une texture finale
5. **Blit** : Copie de la texture debug vers l'écran

**Note** : Le tone mapping est conservé mais commenté (désactivé temporairement)

### 6. Interface utilisateur (GUI)

Ajout d'un menu "G-Buffer Debug" avec 5 modes de visualisation :
- Position (0)
- Normal (1)
- Albedo (2) - mode par défaut
- Metallic (3)
- Roughness (4)

## Compilation et test

```bash
cd build/debug
make
./OM3D
```

## Vérification

Une fois lancé, vous devriez voir :
1. La scène rendue avec les couleurs d'albedo par défaut
2. Un nouveau menu "G-Buffer Debug" dans la barre de menu
3. La possibilité de basculer entre les différentes vues du G-Buffer

## Points importants

- **L'éclairage n'est PAS encore implémenté** : Vous verrez uniquement les données brutes du G-Buffer
- Le tone mapping est désactivé mais le code est conservé (commenté)
- Le shadow mapping est conservé et fonctionnel (pour le futur TP)
- Les textures du G-Buffer sont en RGBA16F pour position/normal (précision) et RGBA8 pour albedo/metal-rough (optimisation)

## Prochaines étapes (TP suivant)

1. Implémenter un shader de lighting qui lit le G-Buffer
2. Réactiver le tone mapping après le lighting
3. Optimiser les formats de texture si nécessaire
