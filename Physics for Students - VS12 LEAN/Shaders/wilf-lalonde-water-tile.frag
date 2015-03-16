#version 120

//
// Fragment shader, Tiled Directional Flow

// Wilf LaLonde's much simplified variation of the shader developed
// in 2010 by frans van hoesel, university of groningen.

uniform sampler2D normalMap;
uniform sampler2D colorMap;
uniform sampler2D flowMap;
uniform samplerCube cubeMap;

varying vec3  Normal;
varying vec3  EyeDir;

varying vec2 texNormal0Coord;
varying vec2 texColorCoord;
varying vec2 texFlowCoord;

varying float myTime;

//NOTES: normalMap is a noise texture of normals (see drawNormalMap)... It seems to be built-in to OpenSceneGraph!!!


#define flowTileFactor 35.0
#define normalFlowTileFactor 10.0

vec2 tile (vec2 uv, float tiles) {return floor (uv) / tiles;}

vec2 sampleFlowVector (vec2 uv) {
   //Note 1: r is x-direction, g is y-direction, b is DOUBLE the velocity in wavelengths, 
   //a is alpha (if there is some, its water).

   //Note 2: Let a:b denote a number in the range a to b. 

   //Normally, a UNIT VECTOR n in the range -1:+1 would have been encoded as m = 0.5*(n+1) 
   //to map to range 0.5 * (-1:+1 + 1) = 0.5 * (0:2) = 0:1 and decoded as 2m-1 to map back; 
   //i.e., from range 2(0:1)-1=(0:2)-1= -1:+1.

   //A SCALED VECTOR of length v is then obtained by multiplying the unit vector by v; i.e. v*(2m-1).
   //But v*(2m-1) = 2v*(m-0.5) which means that by storing 2v in the ".b" component of the vector,
   //we can decode THE UNIT VECTOR via (m-0.5) instead of (2m-1)...

   vec4 pixel = texture2D (flowMap, uv).rgba;
   return pixel.b * (pixel.rg - 0.5);
}
#define sampleFlowAlpha(uv) (texture2D (flowMap, uv).a)

vec2 sampleFlowVector (vec2 uv, float tiles, vec2 tileOffset) {//at tile vertex...
   return sampleFlowVector (tile (uv + tileOffset, tiles));
}

#define time (myTime * 2.0) /*It's deciseconds...*/

vec2 sampleDynamicNormal (vec2 scaledFlowCoordinate, vec2 flowVector) {
    //Rotate to the flow vector and then scroll vertically by the scale factor and time...
    //float scale = 1.0 - (2.0 * length (flowVector)); //flowVector = normalize (flowVector);
    float scale = length (flowVector); 
    mat2 rotationMatrix = mat2 (flowVector.y, flowVector.x, -flowVector.x, flowVector.y);
    //return texture2D (normalMap, rotationMatrix * scaledFlowCoordinate - vec2 (0.0, time * scale * 2.0)).rg * sqrt (abs (scale)) * 2.0;
    return texture2D (normalMap, rotationMatrix * scaledFlowCoordinate - vec2 (0.0, time * scale * 2.0)).rg;
}

#define lerp mix

vec2 interpolateQuad (vec2 A, vec2 B, vec2 C, vec2 D, vec2 position) {
   //Position.x ranges from 0 to 1 from A to B and C to D and position.y ranges from 0 to 1 from A to C and B to D.
   vec2 AB = lerp (A, B, position.x);
   vec2 CD = lerp (C, D, position.x);
   vec2 ABCD = lerp (AB, CD, position.y);
   return ABCD;
} 

vec4 worldColor (float alpha, vec2 normal2D, float normalMapScale) {
    //A straightforward excapsulation of the code found in frans van hoesel's shader...

    //To make the water more transparent, scale the normal with the transparency
    normal2D *= 0.3 * alpha * alpha;
 
    //Assume the normal of plane is 0,0,1 and produce the normalized sum of adding normal2D to it.
    vec3 normal3D = vec3 (normal2D, sqrt (1.0 - dot (normal2D, normal2D)));

    vec3 reflectDirection = reflect (EyeDir, normal3D);
    vec3 envColor = vec3 (textureCube (cubeMap, -reflectDirection)); 
    
    //Very ugly version of fresnel effect but it gives a nice transparent water, but not too transparent.
    float myangle = dot (normal3D, normalize (EyeDir));
    myangle = 0.95 - 0.6 * myangle * myangle;
    
    //Blend in the color of the plane below the water	
    
    //Add in a little distortion of the colormap for the effect of a refracted
    // view of the image below the surface. 
    // (this isn't really tested, just a last minute addition
    // and perhaps should be coded differently
    
    //The correct way, would be to use the refract routine, use the alpha channel for depth of 
    // the water (and make the water disappear when depth = 0), add some watercolor to the colormap
    // depending on the depth, and use the calculated refractdir and the depth to find the right
    // pixel in the colormap.... who knows, something for the next version
    vec3 base = texture2D (colorMap, texColorCoord + vec2 (normal3D * (0.03*alpha/normalMapScale))).rgb;
    return vec4 (lerp (base,envColor, myangle*alpha), 1.0);
}


vec4 perPixelFlowColor (vec2 textureFlowCoord, vec2 scaledNormalFlowCoordinate) {
    //Just to see what happens if we don't use tiling...
    vec2 flow = sampleFlowVector (textureFlowCoord);
    vec2 normal = sampleDynamicNormal (scaledNormalFlowCoordinate, flow); 
    float alpha = sampleFlowAlpha (textureFlowCoord); 
    return vec4 (normal * alpha, 0.0, 1.0);
}


void main () {
    //gl_FragColor = vec4 (texNormal0Coord, 0, 1); return; //0 to 1 in x, 0 to 1 in y.
    //gl_FragColor = vec4 (texColorCoord, 0, 1); return; //0 to 1 in x, 0 to 1 in y.
    //gl_FragColor = vec4 (texFlowCoord, 0, 1); return; //0 to 1 in x, 0 to 1 in y.

    const bool drawTiling = true;
    vec2 scaledFlowCoordinate = texFlowCoord * flowTileFactor;

    if (drawTiling && (fract (scaledFlowCoordinate).x < 0.02 || fract (scaledFlowCoordinate).y < 0.02)) {
	gl_FragColor = vec4 (1,0,0,1); return;
    }

    vec2 flowA = sampleFlowVector (scaledFlowCoordinate, flowTileFactor, vec2 (0.0, 0.0));
    vec2 flowB = sampleFlowVector (scaledFlowCoordinate, flowTileFactor, vec2 (1.0, 0.0));
    vec2 flowC = sampleFlowVector (scaledFlowCoordinate, flowTileFactor, vec2 (0.0, 1.0));
    vec2 flowD = sampleFlowVector (scaledFlowCoordinate, flowTileFactor, vec2 (1.0, 1.0));

    vec2 scaledNormalFlowCoordinate = texFlowCoord * normalFlowTileFactor;
    vec2 normalA = sampleDynamicNormal (scaledNormalFlowCoordinate, flowA); 
    vec2 normalB = sampleDynamicNormal (scaledNormalFlowCoordinate, flowB); 
    vec2 normalC = sampleDynamicNormal (scaledNormalFlowCoordinate, flowC); 
    vec2 normalD = sampleDynamicNormal (scaledNormalFlowCoordinate, flowD); 

    float alpha = sampleFlowAlpha (texFlowCoord);
    vec2 normal2D = interpolateQuad (normalA, normalB, normalC, normalD, fract (scaledFlowCoordinate));
    //gl_FragColor = perPixelFlowColor (texFlowCoord, scaledNormalFlowCoordinate); return; //Just to see per pixel flow...

    gl_FragColor = worldColor (alpha, normal2D, normalFlowTileFactor);
}