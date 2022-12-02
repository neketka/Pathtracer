
#define M_PI (3.1415926535897932384626433832795)

float ggxNormalDistribution( float NdotH, float roughness )
{
	float a2 = roughness * roughness;
	float d = ((NdotH * a2 - NdotH) * NdotH + 1);
	return a2 / (d * d * M_PI);
}

float ggxSchlickMaskingTerm(float NdotL, float NdotV, float roughness)
{
	// Karis notes they use alpha / 2 (or roughness^2 / 2)
	float k = roughness*roughness / 2;

	// Compute G(v) and G(l).  These equations directly from Schlick 1994
	//     (Though note, Schlick's notation is cryptic and confusing.)
	float g_v = NdotV / (NdotV*(1 - k) + k);
	float g_l = NdotL / (NdotL*(1 - k) + k);
	return clamp(g_v * g_l, 0.0, 1.0);
}

vec3 schlickFresnel(vec3 f0, float lDotH)
{
	return f0 + (vec3(1.0f, 1.0f, 1.0f) - f0) * pow(1.0f - lDotH, 5.0f);
}

vec3 getPerpendicularVector(vec3 u)
{
	vec3 a = abs(u);
	uint xm = ((a.x - a.y)<0 && (a.x - a.z)<0) ? 1 : 0;
	uint ym = (a.y - a.z)<0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, vec3(xm, ym, zm));
}

// When using this function to sample, the probability density is:
//      pdf = D * NdotH / (4 * HdotV)
vec3 getGGXMicrofacet(vec2 randVal, float roughness, vec3 hitNorm)
{
	// Get our uniform random numbers

	// Get an orthonormal basis from the normal
  vec3 B = getPerpendicularVector(hitNorm);
	vec3 T = cross(B, hitNorm);

	// GGX NDF sampling
	float a2 = roughness * roughness;
	float cosThetaH = sqrt(max(0.0f, (1.0-randVal.x)/((a2-1.0)*randVal.x+1) ));
	float sinThetaH = sqrt(max(0.0f, 1.0f - cosThetaH * cosThetaH));
	float phiH = randVal.y * M_PI * 2.0f;

	// Get our GGX NDF sample (i.e., the half vector)
	return T * (sinThetaH * cos(phiH)) +
           B * (sinThetaH * sin(phiH)) +
           hitNorm * cosThetaH;
}