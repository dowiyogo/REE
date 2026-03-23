# Prompts para Generar Imagen del Sistema de Densimetría Gamma
## Para usar con Google Gemini, DALL-E, Midjourney, etc.

---

## 🎯 PROMPT RECOMENDADO (Técnico para Presentación)

```
Create a professional technical illustration of a dual-energy gamma-ray densitometry system for rare earth element detection in apatite minerals.

Cross-section view showing three main components arranged horizontally:

LEFT - RADIOACTIVE SOURCE:
- Cylindrical lead-shielded housing (metallic gray)
- Yellow radiation warning symbol
- Contains Am-241 and Na-22 isotopes
- Two gamma ray beams emanating: RED beam (59.5 keV) and BLUE beam (511 keV)

CENTER - MINERAL SAMPLE:
- Rectangular sample holder
- Apatite rock sample with gray-brown crystalline texture
- Visible golden/yellow REE (rare earth element) inclusions scattered throughout
- The RED beam becomes dashed/faded after passing through (high attenuation)
- The BLUE beam passes through with minimal change (low attenuation)

RIGHT - DETECTOR:
- Cylindrical LaBr3 scintillation detector in green/teal housing
- Connected electronics module
- Small display showing gamma spectrum graph

Style: Clean technical illustration, isometric 3D perspective, white background, professional scientific diagram aesthetic. No photorealistic elements - clean schematic style suitable for engineering presentation.

Labels: "Am-241 + Na-22", "59.5 keV", "511 keV", "Apatite + REE", "LaBr₃ Detector"
```

---

## 🔬 PROMPT PARA PAPER CIENTÍFICO (Blanco y Negro)

```
Black and white scientific schematic diagram for academic publication:

Horizontal transmission geometry gamma-ray measurement system cross-section:

- Collimated point source (left): small circle with lead shielding shown as hatched pattern
- Two diverging beam paths labeled γ_low (59.5 keV) and γ_high (511 keV) as dashed lines
- Rectangular sample volume (center): cross-hatched pattern, labeled "Apatite + REE"
- Detector with aperture and crystal (right): simple geometric shapes
- Dimension annotations: d₁ = 5 cm, d₂ = 5 cm, sample thickness x = 1-3 cm
- Coordinate axes in corner

Style: Technical line drawing, no colors, clean vectors, precise geometry, suitable for LaTeX insertion. Journal-quality scientific diagram.
```

---

## 🏭 PROMPT INDUSTRIAL/MINERO

```
Photorealistic visualization of portable nuclear densitometer for mining rare earth elements:

Scene: Modern mining laboratory or processing facility

Main device on sturdy tripod/bench:
- Robust industrial housing (orange/yellow safety colors)
- Lead-shielded source compartment with radiation markings
- Sample chamber with viewing window showing crushed ore sample
- LaBr3 detector module with digital readout
- Connected rugged laptop showing REE concentration analysis software

Environment: Industrial setting with safety equipment visible, professional lighting, slight depth of field. Clean and modern aesthetic emphasizing precision instrumentation.

Style: Industrial product photography, corporate presentation quality, emphasizing safety and technology.
```

---

## 📊 PROMPT INFOGRAFÍA SIMPLE

```
Simple infographic diagram explaining gamma-ray rare earth detection:

Three elements arranged left to right with connecting arrows:

1. GLOWING SOURCE (left)
   - Orange/yellow circular icon
   - Wavy lines representing gamma rays
   - Label: "Radioactive Source"

2. ROCK SAMPLE (center)
   - Gray rock shape with golden sparkles inside
   - Two ray paths: one fading (blocked), one passing through
   - Labels: "Low Energy → Absorbed" and "High Energy → Transmitted"

3. DETECTOR (right)
   - Green sensor icon with small graph display
   - Label: "Detector measures ratio"

Bottom text: "Ratio reveals REE concentration"

Style: Flat design, bold colors, minimal detail, modern corporate infographic aesthetic. Clean white or gradient background.
```

---

## 🇪🇸 PROMPT EN ESPAÑOL

```
Ilustración técnica profesional de sistema de densimetría gamma de doble energía para detección de tierras raras:

Vista en corte mostrando tres componentes principales:

IZQUIERDA - FUENTE RADIACTIVA:
- Contenedor cilíndrico con blindaje de plomo (gris metálico)
- Símbolo de advertencia de radiación (amarillo)
- Isótopos Am-241 y Na-22
- Dos haces de rayos gamma: ROJO (59.5 keV) y AZUL (511 keV)

CENTRO - MUESTRA MINERAL:
- Porta-muestras rectangular
- Muestra de apatito con textura cristalina gris-marrón
- Inclusiones doradas de tierras raras visibles
- El haz ROJO se atenúa (línea discontinua) al atravesar la muestra
- El haz AZUL atraviesa con poca pérdida

DERECHA - DETECTOR:
- Detector de centelleo LaBr3 en carcasa verde/turquesa
- Módulo electrónico conectado
- Pantalla pequeña mostrando espectro gamma

Estilo: Ilustración técnica limpia, perspectiva isométrica 3D, fondo blanco, estética de diagrama científico profesional para presentación de ingeniería.
```

---

## 💻 Código Python para API de Gemini

```python
import google.generativeai as genai

# Configurar
genai.configure(api_key="TU_API_KEY")

# Modelo con capacidad de imagen (verificar disponibilidad)
model = genai.GenerativeModel('gemini-2.0-flash-exp')

prompt = """
[INSERTAR PROMPT AQUÍ]
"""

# Generar
response = model.generate_content(prompt)

# Procesar respuesta con imagen
for part in response.candidates[0].content.parts:
    if hasattr(part, 'inline_data'):
        with open("densimetro_REE.png", "wb") as f:
            f.write(part.inline_data.data)
        print("Imagen guardada!")
```

---

## 🎨 Tips para Mejores Resultados

| Si quieres... | Agrega al prompt... |
|---------------|---------------------|
| Más detalle | "highly detailed, 4K resolution" |
| Sin texto | "no text labels, icons only" |
| Más técnico | "engineering blueprint style" |
| Más comercial | "corporate presentation, modern design" |
| Colores específicos | "color palette: navy blue, orange, white" |

---

## ⚠️ Notas Importantes

1. **Gemini** funciona mejor con prompts en inglés
2. **Iteración**: Si el primer resultado no es ideal, ajusta el prompt
3. **Especificidad**: Cuanto más detallado el prompt, mejor resultado
4. **Estilo**: Especifica claramente si quieres diagrama vs fotorrealista
5. **Verificar licencia**: Confirma que puedes usar la imagen generada comercialmente

---

*Generado para Proyecto Medidor de Tierras Raras - CNP/CORFO*
