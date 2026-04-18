
// initialize
    float vertices[] = {
    // positions          // normals           // texcoords
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f,0.0f,
     0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  1.0f,0.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  1.0f,1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  1.0f,1.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f,1.0f,
    -0.5f,-0.5f,-0.5f,  0.0f, 0.0f,-1.0f,  0.0f,0.0f,

    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  0.0f,0.0f,
     0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f,0.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f,1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  1.0f,1.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  0.0f,1.0f,
    -0.5f,-0.5f, 0.5f,  0.0f, 0.0f, 1.0f,  0.0f,0.0f,

    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  1.0f,0.0f,
    -0.5f, 0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  1.0f,1.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  0.0f,1.0f,
    -0.5f,-0.5f,-0.5f, -1.0f, 0.0f, 0.0f,  0.0f,1.0f,
    -0.5f,-0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  0.0f,0.0f,
    -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f,  1.0f,0.0f,

     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  1.0f,0.0f,
     0.5f, 0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  1.0f,1.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  0.0f,1.0f,
     0.5f,-0.5f,-0.5f,  1.0f, 0.0f, 0.0f,  0.0f,1.0f,
     0.5f,-0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  0.0f,0.0f,
     0.5f, 0.5f, 0.5f,  1.0f, 0.0f, 0.0f,  1.0f,0.0f,

    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  0.0f,1.0f,
     0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  1.0f,1.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  1.0f,0.0f,
     0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  1.0f,0.0f,
    -0.5f,-0.5f, 0.5f,  0.0f,-1.0f, 0.0f,  0.0f,0.0f,
    -0.5f,-0.5f,-0.5f,  0.0f,-1.0f, 0.0f,  0.0f,1.0f,

    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  0.0f,1.0f,
     0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  1.0f,1.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  1.0f,0.0f,
     0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  1.0f,0.0f,
    -0.5f, 0.5f, 0.5f,  0.0f, 1.0f, 0.0f,  0.0f,0.0f,
    -0.5f, 0.5f,-0.5f,  0.0f, 1.0f, 0.0f,  0.0f,1.0f
    };

    Shader lightingShader("../src/shader/backpack/model.vs", "../src/shader/backpack/model.fs");
    Shader lightCubeshader("../src/shader/backpack/lightcube.vs", "../src/shader/backpack/lightcube.fs");
    glEnable(GL_DEPTH_TEST);


    unsigned int lightVAO, lightVBO;
    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //render

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)bfwidth / (float)bfheight, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::vec3 viewPos = camera.Getposition();

        lightingShader.use();
        lightingShader.setMat4("projection", projection);
        lightingShader.setMat4("view", view);
        lightingShader.setVec3("viewPos", viewPos);
        lightingShader.setVec3("pointLights[0].position", lightPos1);
        lightingShader.setVec3("pointLights[0].ambient", glm::vec3(0.02f, 0.02f, 0.02f));
        lightingShader.setVec3("pointLights[0].diffuse", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[0].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[0].constant", 1.0f);
        lightingShader.setFloat("pointLights[0].linear", 0.09f);
        lightingShader.setFloat("pointLights[0].quadratic", 0.032f);

        lightingShader.setVec3("pointLights[1].position", lightPos2);
        lightingShader.setVec3("pointLights[1].ambient", glm::vec3(0.02f, 0.02f, 0.02f));
        lightingShader.setVec3("pointLights[1].diffuse", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[1].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[1].constant", 1.0f);
        lightingShader.setFloat("pointLights[1].linear", 0.09f);
        lightingShader.setFloat("pointLights[1].quadratic", 0.032f);

        lightingShader.setVec3("pointLights[2].position", lightPos3);
        lightingShader.setVec3("pointLights[2].ambient", glm::vec3(0.02f, 0.02f, 0.02f));
        lightingShader.setVec3("pointLights[2].diffuse", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[2].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[2].constant", 1.0f);
        lightingShader.setFloat("pointLights[2].linear", 0.09f);
        lightingShader.setFloat("pointLights[2].quadratic", 0.032f);

        lightingShader.setVec3("pointLights[3].position", lightPos4);
        lightingShader.setVec3("pointLights[3].ambient", glm::vec3(0.02f, 0.02f, 0.02f));
        lightingShader.setVec3("pointLights[3].diffuse", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("pointLights[3].specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("pointLights[3].constant", 1.0f);
        lightingShader.setFloat("pointLights[3].linear", 0.09f);
        lightingShader.setFloat("pointLights[3].quadratic", 0.032f);

        lightingShader.setVec3("flashlight.position", camera.Getposition());
        lightingShader.setVec3("flashlight.direction", camera.GetFront());
        lightingShader.setVec3("flashlight.diffuse", glm::vec3(0.5f, 0.5f, 0.5f));
        lightingShader.setVec3("flashlight.specular", glm::vec3(1.0f, 1.0f, 1.0f));
        lightingShader.setFloat("flashlight.constant", 1.0f);
        lightingShader.setFloat("flashlight.linear", 0.09f);
        lightingShader.setFloat("flashlight.quadratic", 0.032f);
        lightingShader.setFloat("flashlight.cutOff", glm::cos(glm::radians(12.5f)));
        lightingShader.setFloat("flashlight.outerCutOff", glm::cos(glm::radians(17.5f)));
        lightingShader.setInt("flashlightOn", flashLightOn);

        lightingShader.setVec3("dirLight.ambient", glm::vec3(0.05f, 0.05f, 0.05f));
        lightingShader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        lightingShader.setVec3("dirLight.diffuse", glm::vec3(0.2f, 0.2f, 0.2f));
        lightingShader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));

        lightingShader.setFloat("material.shininess", 32.0f);
        

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        lightingShader.setMat4("model", model);
        lightingShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        backpack.Draw(lightingShader);

        lightCubeshader.use();
        lightCubeshader.setMat4("projection", projection);
        lightCubeshader.setMat4("view", view);
        glm::mat4 Lightmodel = glm::mat4(1.0f);
        Lightmodel = glm::translate(Lightmodel, lightPos1);
        Lightmodel = glm::scale(Lightmodel, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", Lightmodel);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 Lightmodel2 = glm::mat4(1.0f);
        Lightmodel2 = glm::translate(Lightmodel2, lightPos2);
        Lightmodel2 = glm::scale(Lightmodel2, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", Lightmodel2);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 Lightmodel3 = glm::mat4(1.0f);
        Lightmodel3 = glm::translate(Lightmodel3, lightPos3);
        Lightmodel3 = glm::scale(Lightmodel3, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", Lightmodel3);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glm::mat4 Lightmodel4 = glm::mat4(1.0f);
        Lightmodel4 = glm::translate(Lightmodel4, lightPos4);
        Lightmodel4 = glm::scale(Lightmodel4, glm::vec3(0.2f)); 
        lightCubeshader.setMat4("model", Lightmodel4);
        glBindVertexArray(lightVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

