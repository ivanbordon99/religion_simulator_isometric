#!/usr/bin/env python3
import os
import struct
from PIL import Image

def process_transparency(im, black_threshold=14, fade_range=28):
    im = im.convert("RGBA")
    data = list(im.getdata())
    out = []
    for r, g, b, a in data:
        max_c = max(r, g, b)
        if max_c <= black_threshold:
            out.append((0, 0, 0, 0))
        elif max_c < black_threshold + fade_range:
            alpha = int(255 * (max_c - black_threshold) / fade_range)
            out.append((r, g, b, alpha))
        else:
            out.append((r, g, b, 255))
    im.putdata(out)
    return im

def save_rgba_and_png(im, base_path):
    im.save(base_path + ".png")
    raw_bytes = im.convert("RGBA").tobytes()
    with open(base_path + ".rgba", "wb") as f:
        f.write(raw_bytes)
    print(f"Saved {base_path}.png and .rgba ({im.size[0]}x{im.size[1]}, {len(raw_bytes)} bytes)")

def main():
    src_path = "/home/Ivancho/.gemini/antigravity/brain/1dbff22e-5d97-4436-b0dc-e7a7cb63301c/.user_uploaded/media_1794277127687_0d20316a.jpg"
    out_dir = "/home/Ivancho/Documentos/religion_simulator_isometric/assets/sprites/fire"
    os.makedirs(out_dir, exist_ok=True)
    
    img = Image.open(src_path)
    print(f"Loaded source image {src_path} ({img.size})")

    # =========================================================================
    # 1. WILDFIRE RAGING INFERNO ANIMATION (4 frames of 128x128 -> 512x128)
    # =========================================================================
    wildfire_sheet = Image.new("RGBA", (512, 128))
    for frame_idx in range(4):
        crop_box = (frame_idx * 128, 896, (frame_idx + 1) * 128, 1024)
        frame_im = process_transparency(img.crop(crop_box))
        wildfire_sheet.paste(frame_im, (frame_idx * 128, 0))
    save_rgba_and_png(wildfire_sheet, os.path.join(out_dir, "wildfire_sheet"))

    # =========================================================================
    # 2. MEDIUM FLAME TONGUES ANIMATION (6 frames of 64x128 -> 384x128)
    # =========================================================================
    flame_sheet = Image.new("RGBA", (384, 128))
    for frame_idx in range(6):
        crop_box = (frame_idx * 64, 512, (frame_idx + 1) * 64, 640)
        frame_im = process_transparency(img.crop(crop_box))
        flame_sheet.paste(frame_im, (frame_idx * 64, 0))
    save_rgba_and_png(flame_sheet, os.path.join(out_dir, "flame_sheet"))

    # =========================================================================
    # 3. SMALL FLAME ANIMATION (FOR BURNING HUMANS & TORCHES) (7 frames of 64x96 -> 448x96)
    # =========================================================================
    small_flame_sheet = Image.new("RGBA", (448, 96))
    for frame_idx in range(7):
        crop_box = (frame_idx * 64, 96, (frame_idx + 1) * 64, 192)
        frame_im = process_transparency(img.crop(crop_box))
        small_flame_sheet.paste(frame_im, (frame_idx * 64, 0))
    save_rgba_and_png(small_flame_sheet, os.path.join(out_dir, "small_flame_sheet"))

    # =========================================================================
    # 4. TRIBAL CAMPFIRES / BONFIRES WITH LOGS (4 frames of 128x128 -> 512x128)
    # =========================================================================
    campfire_sheet = Image.new("RGBA", (512, 128))
    campfire_boxes = [
        (0, 640, 128, 768),
        (128, 640, 256, 768),
        (318, 640, 446, 768),
        (640, 640, 768, 768)
    ]
    for frame_idx, crop_box in enumerate(campfire_boxes):
        frame_im = process_transparency(img.crop(crop_box))
        campfire_sheet.paste(frame_im, (frame_idx * 128, 0))
    save_rgba_and_png(campfire_sheet, os.path.join(out_dir, "campfire_sheet"))

    # =========================================================================
    # 5. METEORS & FIREBALLS (2 frames of 128x128 -> 256x128)
    # =========================================================================
    # Meteor 1: diagonal fireball (768, 640, 896, 768)
    # Meteor 2: giant cosmic comet (896, 768, 1024, 896)
    meteor_sheet = Image.new("RGBA", (256, 128))
    meteor1 = process_transparency(img.crop((768, 640, 896, 768)))
    meteor2 = process_transparency(img.crop((896, 768, 1024, 896)))
    meteor_sheet.paste(meteor1, (0, 0))
    meteor_sheet.paste(meteor2, (128, 0))
    save_rgba_and_png(meteor_sheet, os.path.join(out_dir, "meteor_sheet"))

    # Also individual single meteor files for direct access
    save_rgba_and_png(meteor1, os.path.join(out_dir, "meteor_fireball"))
    save_rgba_and_png(meteor2, os.path.join(out_dir, "meteor_giant"))

    # =========================================================================
    # 6. VOLCANIC ERUPTION FOUNTAIN / MAGMA GEYSER (2 frames of 128x128 -> 256x128)
    # =========================================================================
    volcano_sheet = Image.new("RGBA", (256, 128))
    eruption1 = process_transparency(img.crop((640, 768, 768, 896)))
    eruption2 = process_transparency(img.crop((768, 768, 896, 896)))
    volcano_sheet.paste(eruption1, (0, 0))
    volcano_sheet.paste(eruption2, (128, 0))
    save_rgba_and_png(volcano_sheet, os.path.join(out_dir, "volcano_eruption_sheet"))

    # =========================================================================
    # 7. FIRE PIT / VOLCANIC CRATER VENT (128x128)
    # =========================================================================
    fire_pit = process_transparency(img.crop((640, 640, 768, 768)))
    save_rgba_and_png(fire_pit, os.path.join(out_dir, "fire_pit"))

    # =========================================================================
    # 8. DETONATION FLAME BURSTS (2 frames of 128x128 -> 256x128)
    # =========================================================================
    burst_sheet = Image.new("RGBA", (256, 128))
    burst1 = process_transparency(img.crop((640, 896, 768, 1024)))
    burst2 = process_transparency(img.crop((768, 896, 896, 1024)))
    burst_sheet.paste(burst1, (0, 0))
    burst_sheet.paste(burst2, (128, 0))
    save_rgba_and_png(burst_sheet, os.path.join(out_dir, "flame_burst_sheet"))

    # =========================================================================
    # 9. LAVA & MAGMA TERRAIN TILES & SHORELINE BANKS (512x256 -> 4 tiles 128x128 on Row 0, 4 banks on Row 1)
    # =========================================================================
    # (576, 0): bubbling molten cellular magma
    # (576, 128): liquid boiling lava with viscous bubbles
    # (576, 256): cracked volcanic rock crust
    # (768, 256): basalt bedrock with glowing red magma fissures
    # Row 1: Left/Right shoreline banks and bottom corners with green chromakey removed
    lava_png_src = "/home/Ivancho/.gemini/antigravity/brain/1dbff22e-5d97-4436-b0dc-e7a7cb63301c/.user_uploaded/media_1794664399729_53cc67df.png"
    lava_src = Image.open(lava_png_src) if os.path.exists(lava_png_src) else img

    def clean_green_chromakey(crop_img):
        out = Image.new("RGBA", crop_img.size)
        pix_in = crop_img.load()
        pix_out = out.load()
        w, h = crop_img.size
        for y in range(h):
            for x in range(w):
                r, g, b, a = pix_in[x, y]
                is_green = (g > 115 and g > r + 22 and g > b + 22)
                if is_green:
                    pix_out[x, y] = (0, 0, 0, 0)
                else:
                    if g > r and g > b and r > 30:
                        edge_factor = max(0.0, min(1.0, (g - max(r, b)) / 22.0))
                        clean_g = int(g * (1.0 - edge_factor) + min(r, b) * edge_factor)
                        pix_out[x, y] = (r, clean_g, b, 255)
                    else:
                        pix_out[x, y] = (r, g, b, 255)
        return out

    lava_tiles = Image.new("RGBA", (512, 256))
    # Row 0: 4 solid 128x128 tiles
    t1 = lava_src.crop((576, 0, 704, 128)).convert("RGBA")
    t2 = lava_src.crop((576, 128, 704, 256)).convert("RGBA")
    t3 = lava_src.crop((576, 256, 704, 384)).convert("RGBA")
    t4 = lava_src.crop((768, 256, 896, 384)).convert("RGBA")
    lava_tiles.paste(t1, (0, 0))
    lava_tiles.paste(t2, (128, 0))
    lava_tiles.paste(t3, (256, 0))
    lava_tiles.paste(t4, (384, 0))

    # Row 1: 4 shoreline banks and corners
    bank_left = clean_green_chromakey(lava_src.crop((896, 256, 960, 384)))
    bank_right = clean_green_chromakey(lava_src.crop((704, 256, 768, 384)))
    corner_bl = clean_green_chromakey(lava_src.crop((768, 384, 896, 512)))
    corner_br = clean_green_chromakey(lava_src.crop((896, 384, 1024, 512)))

    lava_tiles.paste(bank_left, (32, 128), bank_left)
    lava_tiles.paste(bank_right, (128 + 32, 128), bank_right)
    lava_tiles.paste(corner_bl, (256, 128), corner_bl)
    lava_tiles.paste(corner_br, (384, 128), corner_br)

    save_rgba_and_png(lava_tiles, os.path.join(out_dir, "lava_tiles"))

    # =========================================================================
    # 10. FULL TRANSPARENT MASTER ATLAS (1024x1024)
    # =========================================================================
    full_atlas = process_transparency(img)
    # Ensure solid terrain tiles in the atlas retain full opacity
    full_atlas.paste(t1, (576, 0))
    full_atlas.paste(t2, (576, 128))
    full_atlas.paste(t3, (576, 256))
    full_atlas.paste(t4, (768, 256))
    save_rgba_and_png(full_atlas, os.path.join(out_dir, "fire_atlas_1024"))

    print("\n[SUCCESS] All fire sprites successfully extracted and converted!")

if __name__ == "__main__":
    main()
