#pragma once
#include "ActorIconSlot.hpp"
#include "SFML/Graphics.hpp"
#include "Graphics/SpriteSheet.hpp"
#include "Verb.hpp"

namespace ng
{
enum class ActorSlotSelectableMode {
  Off = 0,
  On = 1,
  TemporaryUnselectable = 2,
  TemporarySelectable = 3
};

class Engine;
class ActorIcons : public sf::Drawable
{
  public:
    ActorIcons(std::array<ActorIconSlot, 6> &actorsIconSlots, std::array<VerbUiColors, 6> &verbUiColors,
               Actor *&pCurrentActor);

    void setEngine(Engine *pEngine);
    void setMousePosition(const sf::Vector2f &pos);
    void update(const sf::Time &elapsed);
    bool isMouseOver() const { return _isInside; }
    void flash(bool on);
    void setMode(ActorSlotSelectableMode mode);

  private:
    void drawActorIcon(sf::RenderTarget &target, const std::string &icon, int actorSlot, const sf::Vector2f &offset,
                       sf::Uint8 alpha) const;
    void drawActorIcon(sf::RenderTarget &target, const std::string &icon, sf::Color backColor, sf::Color frameColor,
                       const sf::Vector2f &offset, sf::Uint8 alpha) const;
    void draw(sf::RenderTarget &target, sf::RenderStates states) const override;
    int getCurrentActorIndex() const;
    int getIconsNum() const;
    float getOffsetY(int num) const;

  private:
    Engine *_pEngine{nullptr};
    SpriteSheet _gameSheet;
    std::array<ActorIconSlot, 6> &_actorsIconSlots;
    std::array<VerbUiColors, 6> &_verbUiColors;
    Actor *&_pCurrentActor;
    sf::Vector2f _mousePos;
    sf::Clock _clock;
    bool _isInside{false};
    bool _on{true};
    float _position{0};
    bool _isMouseButtonPressed{false};
    sf::Time _time;
    sf::Uint8 _alpha{0};
    ActorSlotSelectableMode _mode{ActorSlotSelectableMode::On};
};
} // namespace ng
