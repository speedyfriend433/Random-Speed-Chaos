#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <random>

using namespace geode::prelude;

class SpeedChaosManager {
public:
    static SpeedChaosManager& get() {
        static SpeedChaosManager instance;
        return instance;
    }

    float currentSpeed = 1.0f;
    float targetSpeed = 1.0f;
    float timeUntilChange = 3.0f;
    float elapsed = 0.0f;
    float transitionProgress = 1.0f;
    bool isInLevel = false;
    std::mt19937 rng{std::random_device{}()};

    void reset() {
        currentSpeed = 1.0f;
        targetSpeed = 1.0f;
        elapsed = 0.0f;
        transitionProgress = 1.0f;
        CCDirector::get()->getScheduler()->setTimeScale(1.0f);
    }

    float getRandomSpeed() {
        float minSpeed = Mod::get()->getSettingValue<double>("min-speed");
        float maxSpeed = Mod::get()->getSettingValue<double>("max-speed");
        std::uniform_real_distribution<float> dist(minSpeed, maxSpeed);
        return dist(rng);
    }

    float getRandomInterval() {
        float minInterval = Mod::get()->getSettingValue<double>("min-interval");
        float maxInterval = Mod::get()->getSettingValue<double>("max-interval");
        std::uniform_real_distribution<float> dist(minInterval, maxInterval);
        return dist(rng);
    }

    std::string getSpeedMessage(float speed) {
        if (speed < 0.3f) return fmt::format(" SNAIL MODE: {:.2f}x", speed);
        if (speed < 0.5f) return fmt::format(" Turtle Time: {:.2f}x", speed);
        if (speed < 0.7f) return fmt::format(" Sleepy: {:.2f}x", speed);
        if (speed < 0.9f) return fmt::format(" Slow Walk: {:.2f}x", speed);
        if (speed < 1.2f) return fmt::format(" Normal-ish: {:.2f}x", speed);
        if (speed < 1.5f) return fmt::format(" Jogging: {:.2f}x", speed);
        if (speed < 1.8f) return fmt::format(" ZOOM ZOOM: {:.2f}x", speed);
        if (speed < 2.5f) return fmt::format(" SONIC SPEED: {:.2f}x", speed);
        if (speed < 3.5f) return fmt::format(" LUDICROUS: {:.2f}x", speed);
        return fmt::format(" INSANITY: {:.2f}x", speed);
    }

private:
    SpeedChaosManager() = default;
};

class $modify(FleroScheduler, CCScheduler) {
    void update(float dt) {
        CCScheduler::update(dt);

        auto& manager = SpeedChaosManager::get();
        
        if (!Mod::get()->getSettingValue<bool>("enabled")) {
            if (manager.currentSpeed != 1.0f) {
                manager.reset();
            }
            return;
        }

        if (Mod::get()->getSettingValue<bool>("only-in-level") && !manager.isInLevel) {
            if (manager.currentSpeed != 1.0f) {
                manager.reset();
            }
            return;
        }

        bool smoothTransition = Mod::get()->getSettingValue<bool>("smooth-transition");
        if (smoothTransition && manager.transitionProgress < 1.0f) {
            float transitionDuration = Mod::get()->getSettingValue<double>("transition-duration");
            manager.transitionProgress += dt / transitionDuration;
            
            if (manager.transitionProgress >= 1.0f) {
                manager.transitionProgress = 1.0f;
                manager.currentSpeed = manager.targetSpeed;
            } else {
                float t = manager.transitionProgress;
                t = t < 0.5f ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
                manager.currentSpeed = manager.currentSpeed + (manager.targetSpeed - manager.currentSpeed) * t;
            }
            
            CCDirector::get()->getScheduler()->setTimeScale(manager.currentSpeed);
        }

        manager.elapsed += dt;
        
        if (manager.elapsed >= manager.timeUntilChange) {
            manager.elapsed = 0.0f;
            
            float newSpeed = manager.getRandomSpeed();
            manager.timeUntilChange = manager.getRandomInterval();
            
            if (smoothTransition) {
                manager.targetSpeed = newSpeed;
                manager.transitionProgress = 0.0f;
            } else {
                manager.currentSpeed = newSpeed;
                manager.targetSpeed = newSpeed;
                CCDirector::get()->getScheduler()->setTimeScale(newSpeed);
            }
            
            if (Mod::get()->getSettingValue<bool>("show-notifications")) {
                Notification::create(
                    manager.getSpeedMessage(newSpeed),
                    NotificationIcon::Info,
                    1.5f
                )->show();
            }
        }
    }
};

class $modify(FleroPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        auto& manager = SpeedChaosManager::get();
        manager.isInLevel = true;
        manager.timeUntilChange = manager.getRandomInterval();
        
        return true;
    }

    void onQuit() {
        auto& manager = SpeedChaosManager::get();
        manager.isInLevel = false;
        manager.reset();
        
        PlayLayer::onQuit();
    }

    void levelComplete() {
        auto& manager = SpeedChaosManager::get();
        manager.reset();
        
        PlayLayer::levelComplete();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        auto& manager = SpeedChaosManager::get();
        manager.reset();
    }
};
