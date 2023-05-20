#version 330

in vec2 UV;
out vec3 color;

uniform sampler2D tex_sampler;

void main(){
	color = texture(tex_sampler, UV).rgb;
}
