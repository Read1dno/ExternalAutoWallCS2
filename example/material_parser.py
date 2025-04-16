import os
import re

def extract_hashes(text):
    pattern = r"m_surfacePropertyHashes\s*=\s*\[(.*?)\]"
    m = re.search(pattern, text, re.DOTALL)
    if not m:
        raise ValueError("Не удалось найти блок m_surfacePropertyHashes в файле.")
    content = m.group(1)
    numbers = re.findall(r'\d+', content)
    return [int(num) for num in numbers]

def extract_blocks(list_content):
    blocks = []
    stack = []
    start = None
    for i, ch in enumerate(list_content):
        if ch == '{':
            if not stack:
                start = i
            stack.append(ch)
        elif ch == '}':
            if stack:
                stack.pop()
                if not stack and start is not None:
                    blocks.append(list_content[start:i+1])
    return blocks

def parse_block(block_str):
    result = {}
    m = re.search(r'surfacePropertyName\s*=\s*"([^"]+)"', block_str)
    if m:
        result['surfacePropertyName'] = m.group(1)
    m = re.search(r'm_nameHash\s*=\s*([0-9]+)', block_str)
    if m:
        result['m_nameHash'] = int(m.group(1))
    m = re.search(r'bulletPenetrationDistanceModifier\s*=\s*([0-9.]+)', block_str)
    if m:
        result['bulletPenetrationDistanceModifier'] = float(m.group(1))
    m = re.search(r'bulletPenetrationDamageModifier\s*=\s*([0-9.]+)', block_str)
    if m:
        result['bulletPenetrationDamageModifier'] = float(m.group(1))
    return result

def parse_file(filepath):
    if not os.path.exists(filepath):
        raise FileNotFoundError(f"Файл {filepath} не найден!")
    with open(filepath, encoding='utf-8') as f:
        return f.read()

def get_surface_properties(m_nameHash):
    txt1 = parse_file("surfaceproperties.txt")
    pattern = r"SurfacePropertiesList\s*=\s*\[(.*?)\]"
    m = re.search(pattern, txt1, re.DOTALL)
    if not m:
        raise ValueError("Не удалось найти блок SurfacePropertiesList в файле surfaceproperties.txt.")
    list_content = m.group(1)
    blocks_str = extract_blocks(list_content)
    blocks = [parse_block(block) for block in blocks_str]
    surface_name = None
    for block in blocks:
        if block.get("m_nameHash") == m_nameHash:
            surface_name = block.get("surfacePropertyName")
            break
    if surface_name is None:
        raise ValueError(f"Свойство с m_nameHash = {m_nameHash} не найдено в surfaceproperties.txt")
    txt2 = parse_file("surfaceproperties_game.txt")
    m2 = re.search(pattern, txt2, re.DOTALL)
    if not m2:
        raise ValueError("Не удалось найти блок SurfacePropertiesList в файле surfaceproperties_game.txt.")
    list_content2 = m2.group(1)
    blocks_str2 = extract_blocks(list_content2)
    game_blocks = [parse_block(block) for block in blocks_str2]
    for block in game_blocks:
        if block.get("surfacePropertyName") == surface_name:
            bullet_distance = block.get("bulletPenetrationDistanceModifier", 0)
            bullet_damage = block.get("bulletPenetrationDamageModifier", 0)
            return (surface_name, bullet_distance, bullet_damage)
    return (surface_name, 0, 0)

directory = os.getcwd()
for filename in os.listdir(directory):
    if filename.endswith(".vphys") or filename.endswith(".phys"):
        try:
            text = parse_file(filename)
            index_m = extract_hashes(text)
            results = []
            for m_hash in index_m:
                try:
                    result = get_surface_properties(m_hash)
                    results.append(result)
                except Exception as e:
                    results.append(str(e))
            print(f"{filename} {results}")
        except Exception as e:
            print(f"{filename} {str(e)}")
