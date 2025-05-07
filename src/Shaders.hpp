const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;
    layout (location = 2) in vec3 aNormal;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform bool isOutline;
    uniform float outlineThickness;
    
    out vec3 FragPos;
    out vec3 Normal;
    out vec3 Color;

    void main() {
        vec3 position = aPos;
        if(isOutline) {
            position += aNormal * outlineThickness;
        }
        
        gl_Position = projection * view * model * vec4(position, 1.0);
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        Color = aColor;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    in vec3 Color;
    uniform vec3 objectColor;
    out vec4 FragColor;
    
    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 viewPos;
    uniform bool isOutline;
    uniform vec3 outlineColor;

    void main() {
        if(isOutline) {
            FragColor = vec4(outlineColor, 1.0);
            return;
        }
        
        // Original lighting calculations
        float ambientStrength = 0.2;
        vec3 ambient = ambientStrength * lightColor;
        
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;
        
        vec3 result = (ambient + diffuse + specular) * objectColor;
        FragColor = vec4(result, 1.0);
    }
)";

const char* laserVertexShader = R"( 
    #version 330 core
    layout (location = 0) in vec3 aPos;
    
    uniform mat4 uViewProj;
    uniform vec3 cameraRight;
    uniform float thickness;
    
    out vec2 uvCoord;

    void main() {
        // Create billboarded quad
        vec3 offset = cameraRight * thickness * ((gl_VertexID % 2) * 2.0 - 1.0);
        vec3 finalPos = aPos + offset;
        gl_Position = uViewProj * vec4(finalPos, 1.0);
        
        // Generate corrected UV coordinates
        uvCoord = vec2(
            float(gl_VertexID) / 3.0,  // U: position along beam (0-1)
            (gl_VertexID % 2) * 2.0 - 1.0  // V: signed width (-1 to 1)
        );
    }
)";

const char* laserFragmentShader = R"(
    #version 330 core
    in vec2 uvCoord;
    out vec4 FragColor;

    uniform vec3 laserColor;
    uniform float alphaFalloff;

    void main() {
        // Calculate distance from center line (circular cross-section)
        float dist = length(vec2(uvCoord.x * 2.0 - 1.0, uvCoord.y * 2.0 - 1.0));
    
        // Core glow with smooth edges
        float alpha = smoothstep(0.8, 0.2, dist); // Inverted for center glow
        alpha *= pow(1.0 - abs(uvCoord.y - 0.5) * 2.0, alphaFalloff); // Length fade
    
        // Color intensity
        float core = smoothstep(0.4, 0.9, 1.0 - dist);
    
        FragColor = vec4(laserColor * (1.0 + core * 2.0), alpha);
        if(FragColor.a < 0.05) discard;
    }
)";

const char* hudVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aUV;
    out vec2 uv;
    uniform vec2 position;
    
    void main() {
        uv = aUV;
        gl_Position = vec4(aPos + position, 0.0, 1.0);
    }
)";

const char* hudFragmentShader = R"(
    #version 330 core
    in vec2 uv;
    out vec4 FragColor;
    uniform sampler2D tex;
    
    void main() {
        FragColor = texture(tex, uv);
        if(FragColor.a < 0.1) discard;
    }
)";

const char* hudVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aUV;
    out vec2 uv;
    uniform vec2 position;
    uniform vec2 scale;
    
    void main() {
        uv = aUV;
        vec2 scaledPos = aPos * scale;
        gl_Position = vec4(scaledPos + position, 0.0, 1.0);
    }
)";

const char* hudFragmentShaderSource = R"(
    #version 330 core
    in vec2 uv;
    out vec4 FragColor;
    uniform sampler2D tex;
    uniform float alpha;  // Keep this uniform
    
    void main() {
        vec4 texColor = texture(tex, uv);
        texColor.a *= alpha;  // Apply alpha modulation
        if(texColor.a < 0.1) discard;
        FragColor = texColor;
    }
)";

const char* particleVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec4 aColor;
    
    uniform float pointSize = 8.0;
    uniform mat4 uViewProj;
    
    out vec4 vColor;
    
    void main() {
        gl_Position = uViewProj * vec4(aPos, 1.0);
        gl_PointSize = pointSize;
        vColor = aColor;
    }
)";

const char* particleFragmentShader = R"(
    #version 330 core
    in vec4 vColor;
    out vec4 FragColor;
    uniform sampler2D uTexture;
    
    void main() {
        FragColor = texture(uTexture, gl_PointCoord) * vColor;
        if(FragColor.a < 0.1) discard;
    }
)";

const char* textVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
    out vec2 TexCoords;
    
    uniform mat4 projection;
    
    void main() {
        gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
        TexCoords = vertex.zw;
    }
    )";
    
    const char* textFragmentShader = R"(
    #version 330 core
    in vec2 TexCoords;
    out vec4 FragColor;
    
    uniform sampler2D text;
    uniform vec3 textColor;
    
    void main() {    
        vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
        FragColor = vec4(textColor, 1.0) * sampled;
    }
)";

const char* uiVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aUV;

    uniform mat4 projection;
    uniform vec2 position;
    uniform vec2 size;

    out vec2 uv;

    void main() {
    vec2 scaledPos = aPos * size + position;
        gl_Position = projection * vec4(scaledPos, 0.0, 1.0);
        uv = aUV;
    }
)";

const char* uiFragmentShader = R"(
    #version 330 core
    in vec2 uv;
    out vec4 FragColor;

    uniform sampler2D tex;
    uniform vec4 color;
    uniform float alpha;

    void main() {
        vec4 texColor = texture(tex, uv);
        FragColor = texColor * vec4(color.rgb, color.a * alpha);
        if(FragColor.a < 0.1) discard;
    }
)";