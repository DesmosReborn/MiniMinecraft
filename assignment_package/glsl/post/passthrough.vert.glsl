#version 150
// ^ Change this to version 130 if you have compatibility issues

// For post-process shaders, all we have to do is pass down the position and UV data

in vec4 vs_Pos;
in vec2 vs_UV;

out vec2 fs_UV;

void main()
{
    gl_Position = vs_Pos;
    fs_UV = vs_UV;
}
