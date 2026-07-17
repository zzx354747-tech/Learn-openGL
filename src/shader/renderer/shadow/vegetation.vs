#version 330 core
layout(location=0) in vec3 aPosition;
layout(location=3) in vec2 aWindVariation;
layout(location=5) in vec4 iPosScale;
layout(location=6) in vec4 iRotColor;
uniform mat4 lightSpaceMatrix;
uniform float u_time;
uniform float u_windSpeed;
uniform float u_windStrength;
uniform vec2 u_windDir;
float hash12(vec2 p){vec3 p3=fract(vec3(p.xyx)*.1031);p3+=dot(p3,p3.yzx+33.33);return fract((p3.x+p3.y)*p3.z);}
mat3 rotationY(float a){float c=cos(a),s=sin(a);return mat3(c,0,-s,0,1,0,s,0,c);}
mat3 tiltRotation(float a,float z){vec3 x=normalize(vec3(-sin(z),0,cos(z)));float c=cos(a),s=sin(a);mat3 k=mat3(0,x.z,-x.y,-x.z,0,x.x,x.y,-x.x,0);return mat3(1)+s*k+(1-c)*k*k;}
void main(){vec3 s=vec3(mix(.88,1.12,hash12(iPosScale.xz*.071)),mix(.92,1.10,hash12(iPosScale.zx*.113+17)),mix(.88,1.12,hash12(iPosScale.xz*.157+41)));vec3 local=aPosition*mix(vec3(1),s,aWindVariation.y)*iPosScale.w;vec3 world=iPosScale.xyz+tiltRotation(iRotColor.y,iRotColor.z)*rotationY(iRotColor.x)*local;vec2 d=length(u_windDir)>.0001?normalize(u_windDir):vec2(1,0);float p=dot(iPosScale.xz,vec2(.13,.17))+u_time*u_windSpeed;vec2 o=d*(sin(p)+.5*sin(p*2.3))*u_windStrength*aWindVariation.x*iPosScale.w;world.xz+=o;world.y-=length(o)*.035*aWindVariation.x;gl_Position=lightSpaceMatrix*vec4(world,1);}
