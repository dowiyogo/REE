# ============================================================================
# PROMPTS PARA GENERAR IMAGEN DEL SISTEMA DE DENSIMETRÍA GAMMA DUAL-ENERGY
# Para usar con Google Gemini (Imagen FX o API)
# ============================================================================

# =============================================================================
# PROMPT 1: Diagrama Técnico-Científico (Recomendado para presentación técnica)
# =============================================================================

PROMPT_TECNICO = """
Create a professional technical illustration of a dual-energy gamma-ray densitometry system for mineral analysis, shown in cross-section view:

LEFT SIDE: A cylindrical radioactive source housing (metallic gray with yellow radiation warning symbol), containing Am-241 and Na-22 isotopes. Show gamma rays emanating as two distinct colored beams - red for low energy (59.5 keV) and blue for high energy (511 keV).

CENTER: A rectangular mineral sample holder containing crusite/apatite rock sample (gray-brown crystalline texture with visible golden/yellow REE (rare earth element) inclusions scattered throughout).

RIGHT SIDE: A cylindrical scintillation detector (LaBr3 crystal) in green/teal housing, connected to electronics module showing a simple spectrum display.

Style: Clean technical illustration, isometric or slight 3D perspective, white/light gray background, labeled components, professional scientific diagram aesthetic similar to those found in physics textbooks. No photorealistic elements - keep it as a clear schematic diagram.

Include subtle arrows showing gamma ray paths through the sample, with the red beam being more attenuated (dashed/faded) after passing through the sample compared to the blue beam.
"""

# =============================================================================
# PROMPT 2: Visualización 3D Realista (Para impacto visual)
# =============================================================================

PROMPT_3D_REALISTA = """
Photorealistic 3D render of a portable nuclear densitometry device for mining applications:

A compact industrial instrument on a laboratory bench showing:
- Left module: Lead-shielded radioactive source container (cylindrical, dark gray metal with caution markings)
- Center: Sample chamber with transparent window showing a mineral rock sample (apatite with visible crystalline structure)
- Right module: Radiation detector with connected digital display showing a gamma spectrum graph
- Connecting cables and a laptop computer showing analysis software

Environment: Modern mining laboratory setting, clean and professional lighting, depth of field effect focusing on the instrument.

Style: Industrial product photography aesthetic, high detail, professional lighting with soft shadows, colors emphasizing safety (yellow/black hazard markings) and technology (blue LED indicators).
"""

# =============================================================================
# PROMPT 3: Diagrama Conceptual Simplificado (Para audiencia no técnica)
# =============================================================================

PROMPT_SIMPLE = """
Simple infographic-style diagram showing how gamma rays detect rare earth elements in rocks:

Three main elements arranged horizontally with arrows between them:

1. SOURCE (left): Orange/yellow glowing circle labeled "Radioactive Source" with wavy lines (gamma rays) pointing right

2. SAMPLE (center): Gray rock/mineral chunk with sparkly golden dots inside representing rare earth elements. Two types of rays passing through - one getting blocked more (shown fading), one passing through easily

3. DETECTOR (right): Green rectangular sensor with a small graph/display showing detected signals

Background: Clean white or light blue gradient
Style: Flat design, minimal, corporate infographic style with bold colors and clean lines. Modern vector illustration aesthetic.

Add small icons or labels: "Low Energy → Blocked by REE" and "High Energy → Passes Through"
"""

# =============================================================================
# PROMPT 4: Esquema tipo Paper Científico
# =============================================================================

PROMPT_PAPER = """
Black and white scientific schematic diagram suitable for academic publication:

Horizontal cross-section view of transmission geometry gamma-ray measurement system:

- Collimated point source on left (shown as small circle with lead shielding indicated by hatched pattern)
- Two diverging beam paths (dashed lines) labeled γ_low and γ_high
- Rectangular sample volume in center (cross-hatched to indicate solid material) labeled "Apatite + REE matrix"
- Detector aperture and crystal on right (simple geometric shapes)
- Dimension arrows showing typical distances (source-sample: 5cm, sample-detector: 5cm, sample thickness: 1-3cm)
- Coordinate axes (x,y) in corner

Style: Technical line drawing, no colors, suitable for LaTeX/scientific paper insertion. Clean vectors, precise geometry, minimal artistic elements. Similar to diagrams in Nuclear Instruments and Methods journal.
"""

# =============================================================================
# PROMPT 5: Aplicación Industrial/Minera
# =============================================================================

PROMPT_INDUSTRIAL = """
Industrial mining application visualization:

Show a rugged portable device being used in a mining environment:
- A handheld or tripod-mounted gamma densitometer pointing at a conveyor belt carrying crushed ore
- The device has a robust orange/yellow industrial housing
- Digital display showing real-time REE concentration reading
- A mining engineer in safety gear (hard hat, vest) operating the device
- Background: Open pit mine or processing plant setting, slightly blurred

Style: Realistic industrial photography style, emphasizing practical field application. Warm lighting suggesting outdoor mining environment. Professional corporate/industrial aesthetic.
"""

# =============================================================================
# CÓDIGO PYTHON PARA USAR CON GEMINI API
# =============================================================================
"""
Para usar estos prompts con la API de Google Gemini:

1. Instalar: pip install google-generativeai

2. Código de ejemplo:
"""

CODIGO_PYTHON = '''
import google.generativeai as genai
from pathlib import Path

# Configurar API key
genai.configure(api_key="TU_API_KEY_AQUI")

# Seleccionar modelo con capacidad de generación de imágenes
# Nota: Verificar el modelo actual disponible para imagen
model = genai.GenerativeModel('gemini-2.0-flash-exp')  # o el modelo de imagen disponible

# Prompt para generar
prompt = """
Create a professional technical illustration of a dual-energy gamma-ray 
densitometry system for rare earth element detection in apatite minerals.

Show in cross-section view:
- LEFT: Cylindrical source housing (Am-241 + Na-22) with radiation symbol
- CENTER: Rock sample with visible REE inclusions (golden particles in gray matrix)  
- RIGHT: LaBr3 scintillation detector (green/teal) with spectrum display

Two gamma ray beams: red (59.5 keV, more attenuated) and blue (511 keV, less attenuated)

Style: Clean technical diagram, isometric view, white background, labeled components,
professional scientific illustration for mining industry presentation.
"""

# Generar imagen
response = model.generate_content(prompt)

# Guardar imagen si está disponible
if response.candidates[0].content.parts:
    for i, part in enumerate(response.candidates[0].content.parts):
        if hasattr(part, 'inline_data'):
            image_data = part.inline_data.data
            with open(f"densimetro_REE_{i}.png", "wb") as f:
                f.write(image_data)
            print(f"Imagen guardada: densimetro_REE_{i}.png")
'''

# =============================================================================
# PROMPT EN ESPAÑOL (Por si Gemini lo procesa mejor)
# =============================================================================

PROMPT_ESPANOL = """
Crea una ilustración técnica profesional de un sistema de densimetría gamma de doble energía para análisis de minerales:

Vista en corte transversal mostrando:

IZQUIERDA: Contenedor cilíndrico de fuente radiactiva (gris metálico con símbolo de radiación amarillo), conteniendo isótopos Am-241 y Na-22. Muestra rayos gamma como dos haces de colores distintos - rojo para baja energía (59.5 keV) y azul para alta energía (511 keV).

CENTRO: Porta-muestras rectangular con muestra de roca de apatito (textura cristalina gris-marrón con inclusiones doradas/amarillas de tierras raras dispersas).

DERECHA: Detector de centelleo cilíndrico (cristal LaBr3) en carcasa verde/turquesa, conectado a módulo electrónico mostrando un espectro simple.

Estilo: Ilustración técnica limpia, perspectiva isométrica o 3D suave, fondo blanco/gris claro, componentes etiquetados, estética de diagrama científico profesional similar a los de libros de física.

Incluir flechas sutiles mostrando la trayectoria de los rayos gamma a través de la muestra, con el haz rojo más atenuado (discontinuo/desvanecido) después de pasar por la muestra comparado con el haz azul.
"""

# =============================================================================
# CONSEJOS PARA MEJORES RESULTADOS
# =============================================================================
"""
TIPS PARA GEMINI:

1. Si el primer resultado no es satisfactorio, agregar:
   - "highly detailed"
   - "4K resolution" 
   - "professional quality"
   - "no text overlays" (si no quieres texto generado)

2. Para estilo más técnico, agregar:
   - "technical drawing style"
   - "engineering diagram"
   - "blueprint aesthetic"

3. Para estilo más comercial/presentación:
   - "corporate presentation style"
   - "modern infographic"
   - "clean minimal design"

4. Especificar lo que NO quieres:
   - "no cartoonish elements"
   - "no fantasy elements"
   - "no humans in the image"
   - "no watermarks"

5. Si genera texto ilegible, pedir:
   - "labels in separate text boxes"
   - "minimal text, icons instead"
"""

print("="*70)
print("PROMPTS PARA GEMINI - SISTEMA DENSIMETRÍA GAMMA")
print("="*70)
print("\nSelecciona el prompt más adecuado para tu audiencia:")
print("1. PROMPT_TECNICO    - Diagrama técnico-científico")
print("2. PROMPT_3D_REALISTA - Render 3D fotorrealista")
print("3. PROMPT_SIMPLE     - Infografía simplificada")
print("4. PROMPT_PAPER      - Esquema para paper científico")
print("5. PROMPT_INDUSTRIAL - Aplicación minera industrial")
print("6. PROMPT_ESPANOL    - Versión en español")
print("\nCopia el prompt deseado y pégalo en Gemini o usa el código Python.")
