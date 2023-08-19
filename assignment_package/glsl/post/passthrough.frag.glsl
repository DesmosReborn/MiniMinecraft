#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec2 fs_UV;

uniform sampler2D u_Texture;

out vec4 out_Col;

void main()
{
    out_Col = vec4(texture(u_Texture, fs_UV).rgb, 1);
}
