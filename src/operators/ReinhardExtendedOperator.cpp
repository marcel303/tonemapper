/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Tonemap.h>

namespace tonemapper {

class ReinhardExtendedOperator : public TonemapOperator {
public:
    ReinhardExtendedOperator() : TonemapOperator() {
        name = "Reinhard (Extended)";
        description = R"(Extended mapping proposed in "Photographic Tone
            Reproduction for Digital Images" by Reinhard et al. 2002. An
            additional user parameter specifies the smallest luminance that is
            mapped to 1, which allows high luminances to burn out.)";

        fragmentShader = R"glsl(
            #version 330

            in vec2 uv;
            out vec4 out_color;
            uniform sampler2D source;
            uniform float exposure;
            uniform float gamma;
            uniform float Lwhite;

            float luminance(vec3 color) {
                return 0.212671 * color.r + 0.71516 * color.g + 0.072169 * color.b;
            }

            void main() {
                // Fetch color and convert to luminance
                vec3 Cin = exposure * texture(source, uv).rgb;
                float Lin = luminance(Cin);

                // Apply exposure scale to parameters
                float Lwhite_ = exposure * Lwhite;

                // Apply tonemapping curve to luminance
                float Lout = (Lin * (1.0 + Lin / (Lwhite_ * Lwhite_))) / (1.0 + Lin);

                // Treat color by preserving color ratios [Schlick 1994].
                vec3 Cout = Cin / Lin * Lout;

                // Apply gamma curve and clamp
                Cout = pow(Cout, vec3(1.0 / gamma));
                Cout = clamp(Cout, 0.0, 1.0);
                out_color = vec4(Cout, 1.0);
            }
        )glsl";

        parameters["gamma"] = Parameter(2.2f, 0.f, 10.f, "gamma", "Gamma correction value.");
    }

    void preprocess(const Image *image) override {
        float min = image->getMinimumLuminance(),
              max = image->getMaximumLuminance(),
              start = 0.5f*(min + max);
        parameters["Lwhite"] = Parameter(start, min, max, "Lwhite", "Smallest luminance that is mapped to 1.");
    }

    Color3f map(const Color3f &color, float exposure) const override {
        // Fetch parameters
        float gamma  = parameters.at("gamma").value,
              Lwhite = parameters.at("Lwhite").value;

        // Fetch color and convert to luminance
        Color3f Cin = exposure * color;
        float Lin = luminance(Cin);

        // Apply exposure scale to parameters
        Lwhite *= exposure;

        // Apply tonemapping curve to luminance
        float Lout = (Lin * (1.f + Lin / (Lwhite * Lwhite))) / (1.f + Lin);

        // Treat color by preserving color ratios [Schlick 1994].
        Color3f Cout = Cin / Lin * Lout;

        // Apply gamma curve and clamp
        Cout = pow(Cout, 1.f / gamma);
        return clamp(Cout, 0.f, 1.f);
    }
};

REGISTER_OPERATOR(ReinhardExtendedOperator, "reinhard_extended");

} // Namespace tonemapper
