#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

#include "block.hpp"
#include "error.hpp"
#include "img.hpp"
#include "loader.hpp"
#include "stb_image_write.h"

int color_dist(rgba x, rgba y) {
    return std::pow(x.r - y.r, 2) + std::pow(x.g - y.g, 2) + std::pow(x.b - y.b, 2);
}

struct args {
    std::string textures_dir;
    std::string colors;
    std::string image_path;
    std::string out_dir;
};

std::optional<args> parse_args(char* argv[]) {
    try {
        return args{argv[1], argv[2], argv[3], argv[4]};
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

int main(int argc, char* argv[]) {
    auto args = parse_args(argv);
    if (!args.has_value()) {
        print_error("invalid arguments");
        return 1;
    }

    auto loader = load_block_colors(args->textures_dir, args->colors);
    if (!loader.has_value()) return 1;

    const auto& blocks = loader.value();

    auto loaded_img = load_img(args->image_path);

    if (loaded_img.has_value()) {
        const auto pixels = loaded_img->extract_pixels();

        u_char* new_img_ptr = static_cast<u_char*>(malloc(loaded_img->w * loaded_img->h * 16 * 16 * loaded_img->comp));
        img new_img(loaded_img->w * 16, loaded_img->h * 16, loaded_img->comp, new_img_ptr);

        std::ofstream report_file(args->out_dir + "/report.txt");

        for (int x{}; x < loaded_img->w; x++) {
            for (int y{}; y < loaded_img->h; y++) {
                auto px = pixels.at(x * loaded_img->h + y);
                int least_dist = -1;
                int best_block_idx;
                for (int i{}; i < blocks.size(); i++) {
                    const auto& block = blocks[i];

                    int dist = color_dist(block.color, px.color);
                    if (least_dist < 0) {
                        best_block_idx = i;
                        least_dist = dist;
                    }
                    if (dist < least_dist) {
                        best_block_idx = i;
                        least_dist = dist;
                    }
                }

                const auto& b = blocks.at(best_block_idx);

                report_file << "(" + std::to_string(x) + ", " + std::to_string(y) + "): " << '\n';
                report_file << "(" + std::to_string(px.x) + ", " + std::to_string(px.y) + "): " << '\n';
                report_file << "     " + px.color.to_string() << '\n';
                report_file << "     " + b.name << '\n';
                report_file << "=========================" << '\n';
                new_img.insert_img_at(b.image, x * 16, y * 16);
            }
        }
        stbi_write_png((args->out_dir + "/output.png").c_str(), new_img.w, new_img.h, new_img.comp, new_img.data.get(), new_img.w * new_img.comp);
        report_file.close();
    } else {
        print_error("couldn't load textures");
    }
    return 0;
}