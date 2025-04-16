#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cmath>
#include <tuple>
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include "VisCheck.h"
#include "Math.hpp"
#include <iostream>

namespace py = pybind11;

float distance(const Vector3& a, const Vector3& b) {
    return std::sqrt(
        (a.x - b.x) * (a.x - b.x) +
        (a.y - b.y) * (a.y - b.y) +
        (a.z - b.z) * (a.z - b.z)
    );
}

class VisCheckWrapper {
public:
    VisCheckWrapper(const std::string& phys_path) : checker(phys_path) {}

    float handle_bullet_penetration(
        const std::tuple<float, float, float>& shooter,
        const std::tuple<float, float, float>& target,
        float base_damage,
        float range_modifier,
        float penetration_power,
        const std::vector<std::tuple<std::string, float, float>>& material_data
    ) {
        if (material_data.empty()) {
            throw std::invalid_argument("material_data is empty");
        }

        Vector3 shooter_pos{ std::get<0>(shooter), std::get<1>(shooter), std::get<2>(shooter) };
        Vector3 target_pos{ std::get<0>(target), std::get<1>(target), std::get<2>(target) };

        float current_damage = base_damage;
        std::unordered_map<int, std::tuple<std::string, float, float>> material_dict;
        for (size_t i = 0; i < material_data.size(); ++i)
            material_dict[static_cast<int>(i)] = material_data[i];

        auto hit_data = checker.IsPointVisible(shooter_pos, target_pos);
        if (hit_data.empty()) {
            float total_distance = distance(shooter_pos, target_pos);
            current_damage *= std::pow(range_modifier, total_distance / 500.0f);
            return std::max(current_damage, 0.0f);
        }

        Vector3 current_pos = shooter_pos;
        for (const auto& hit : hit_data) {
            float dist_to_hit = std::get<0>(hit);
            int entry_mat_idx = std::get<1>(hit);
            float inside_distance = std::get<2>(hit);
            inside_distance *= 2.54f;
            int exit_mat_idx = std::get<3>(hit);

            float step_distance = dist_to_hit - distance(shooter_pos, current_pos);
            current_damage *= std::pow(range_modifier, step_distance / 500.0f);

            auto entry = material_dict.count(entry_mat_idx) ? material_dict[entry_mat_idx] : material_dict[15];
            auto exit_ = material_dict.count(exit_mat_idx) ? material_dict[exit_mat_idx] : material_dict[15];

            float entry_pen_mod = std::get<1>(entry);
            float exit_pen_mod = std::get<1>(exit_);
            float combined_pen_mod;

            std::string entry_name = std::get<0>(entry);
            if (entry_mat_idx == exit_mat_idx) {
                if (entry_name == "cardboard" || entry_name == "Wood" || entry_name == "Wood_Plank")
                    combined_pen_mod = 3.0f;
                else if (entry_name == "plastic" || entry_name == "plastic_barrel" || entry_name == "plastic_solid")
                    combined_pen_mod = 2.0f;
                else
                    combined_pen_mod = (entry_pen_mod + exit_pen_mod) / 2.0f;
            }
            else {
                combined_pen_mod = (entry_pen_mod + exit_pen_mod) / 2.0f;
            }

            if (combined_pen_mod == 0.0f)
                return 0.0f;

            float modifier = std::max(0.0f, 1.0f / combined_pen_mod);
            float lost_damage = std::max(
                ((modifier * inside_distance * inside_distance) / 24.0f) +
                ((current_damage * 0.18f) + std::max(3.75f / penetration_power, 0.0f) * 3.0f * modifier),
                0.0f
            );

            if (lost_damage > current_damage)
                return 0.0f;
            current_damage -= lost_damage;
            if (current_damage < 1.0f)
                return 0.0f;

            current_pos.z += inside_distance;
        }

        float remaining_distance = distance(current_pos, target_pos);
        current_damage *= std::pow(range_modifier, remaining_distance / 500.0f);
        return std::max(current_damage, 0.0f);
    }

private:
    VisCheck checker;
};

PYBIND11_MODULE(AutoWall, m) {
    py::class_<VisCheckWrapper>(m, "VisCheck")
        .def(py::init<const std::string&>())
        .def("handle_bullet_penetration",
            &VisCheckWrapper::handle_bullet_penetration,
            py::arg("shooter"),
            py::arg("target"),
            py::arg("base_damage"),
            py::arg("range_modifier"),
            py::arg("penetration_power"),
            py::arg("material_data"));
}
