import AutoWall

checker = AutoWall.VisCheck("de_vertigo.phys")
damage = checker.handle_bullet_penetration(
    (x1, y1, z1), # me
    (x2, y2, z2), # enemy
    base_damage=299,
    range_modifier=0.98,
    penetration_power=250,
    material_data=[('metal', 0.4, 0), ('wood', 0.9, 0.6), …] # see material_parser.py
)
print(f"Calculated damage: {damage}")
