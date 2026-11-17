#!/usr/bin/env python3
"""
Extract volcano animation sprites and sheets from the user-uploaded pixel-art image.
Generates RGBA binary files and PNG previews.
"""
from PIL import Image
import os
import struct

def main():
    src_path = "/home/Ivancho/.gemini/antigravity/brain/1dbff22e-5d97-4436-b0dc-e7a7cb63301c/.user_uploaded/media_1794280824362_bbcc6a2d.png"
    out_dir = "/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/volcano"
    os.makedirs(out_dir, exist_ok=True)

    img = Image.open(src_path).convert("RGBA")
    w, h = img.size
    print(f"[Tool] Source image loaded: {w}x{h}")

    # 1. Export full sheet as RGBA and PNG
    full_rgba_path = os.path.join(out_dir, "volcano_sheet.rgba")
    full_png_path = os.path.join(out_dir, "volcano_sheet.png")
    img.save(full_png_path)

    with open(full_rgba_path, "wb") as f:
        f.write(img.tobytes())
    print(f"[Tool] Saved full atlas: {full_rgba_path} ({os.path.getsize(full_rgba_path)} bytes)")

    # 2. Extract Dormant Stratovolcano (Row 0, Col 4: 819..1024, 0..153)
    dormant = img.crop((819, 0, 1024, 153))
    dormant.save(os.path.join(out_dir, "volcano_dormant.png"))
    with open(os.path.join(out_dir, "volcano_dormant.rgba"), "wb") as f:
        f.write(dormant.tobytes())

    # 3. Extract Small Fumarole / Cinder Cone (Row 0, Col 2: 409..614, 0..153)
    fumarole = img.crop((409, 0, 614, 153))
    fumarole.save(os.path.join(out_dir, "volcano_fumarole.png"))
    with open(os.path.join(out_dir, "volcano_fumarole.rgba"), "wb") as f:
        f.write(fumarole.tobytes())

    # 4. Extract Awakening / Heating Caldera Sheet (Row 1: 0..1024, 153..306)
    awakening = img.crop((0, 153, 1024, 306))
    awakening.save(os.path.join(out_dir, "volcano_awakening_sheet.png"))
    with open(os.path.join(out_dir, "volcano_awakening_sheet.rgba"), "wb") as f:
        f.write(awakening.tobytes())

    # 5. Extract Explosive Eruption Blast Sheet (Rows 2 & 3: 0..1024, 306..612)
    eruption = img.crop((0, 306, 1024, 612))
    eruption.save(os.path.join(out_dir, "volcano_eruption_blast_sheet.png"))
    with open(os.path.join(out_dir, "volcano_eruption_blast_sheet.rgba"), "wb") as f:
        f.write(eruption.tobytes())

    # 6. Extract Lava Rivers & Base Flow Sheet (Row 4: 0..1024, 612..765)
    lava_flow = img.crop((0, 612, 1024, 765))
    lava_flow.save(os.path.join(out_dir, "volcano_lava_flow_sheet.png"))
    with open(os.path.join(out_dir, "volcano_lava_flow_sheet.rgba"), "wb") as f:
        f.write(lava_flow.tobytes())

    print("[Tool] Volcano extraction complete!")

if __name__ == "__main__":
    main()
