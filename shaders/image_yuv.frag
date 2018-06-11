#version 400

in vec2 TexCoord0;

layout( location = 0 ) out vec4 FragColor;

uniform sampler2D UYVYtex; 	// UYVY macropixel texture passed as RGBA format

void main(void)
{
	float tx, ty, Y, Cb, Cr, r, g, b;
	tx = TexCoord0.x;
	ty = TexCoord0.y;

	// The UYVY texture appears to the shader with 1/2 the true width since we used RGBA format to pass UYVY
	int true_width = textureSize(UYVYtex, 0).x * 2;

	// For U0 Y0 V0 Y1 macropixel, lookup Y0 or Y1 based on whether
	// the original texture x coord is even or odd.
	if (fract(floor(tx * true_width + 0.5) / 2.0) > 0.0)
		Y = texture2D(UYVYtex, TexCoord0).a;		// odd so choose Y1
	else
		Y = texture2D(UYVYtex, TexCoord0).g;		// even so choose Y0
	Cb = texture2D(UYVYtex, TexCoord0).b;
	Cr = texture2D(UYVYtex, TexCoord0).r;

	// Y: Undo 1/256 texture value scaling and scale [16..235] to [0..1] range
	// C: Undo 1/256 texture value scaling and scale [16..240] to [-0.5 .. + 0.5] range
	Y = (Y * 256.0 - 16.0) / 219.0;
	Cb = (Cb * 256.0 - 16.0) / 224.0 - 0.5;
	Cr = (Cr * 256.0 - 16.0) / 224.0 - 0.5;
	// Convert to RGB using Rec.709 conversion matrix (see eq 26.7 in Poynton 2003)
	r = Y + 1.5748 * Cr;
	g = Y - 0.1873 * Cb - 0.4681 * Cr;
	b = Y + 1.8556 * Cb;

	// Set alpha to 0.7 for partial transparency when GL_BLEND is enabled
	FragColor = vec4(r, g, b, 1.0);
};