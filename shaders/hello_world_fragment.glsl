#version 100

void main() {
    gl_FragColor = vec4(gl_FragCoord.xy / vec2(1280,720), 0, 1);
}