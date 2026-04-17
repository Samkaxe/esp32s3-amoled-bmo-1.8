#pragma once
#include <Arduino.h>

class Scene {
public:
    virtual void init() = 0;
    virtual void update(uint32_t now) = 0;
    virtual void deinit() = 0;
    virtual ~Scene() {}
};

class SceneManager {
public:
    static void switchScene(Scene* newScene);
    static void update(uint32_t now);
private:
    static Scene* currentScene;
};
