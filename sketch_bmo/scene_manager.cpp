#include "scene_manager.h"

Scene* SceneManager::currentScene = nullptr;

void SceneManager::switchScene(Scene* newScene) {
    if (currentScene) {
        currentScene->deinit();
        delete currentScene;
    }
    currentScene = newScene;
    if (currentScene) {
        currentScene->init();
    }
}

void SceneManager::update(uint32_t now) {
    if (currentScene) {
        currentScene->update(now);
    }
}
