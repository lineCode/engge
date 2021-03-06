#pragma once
#include <memory>
#include <optional>
#include "squirrel.h"
#include "Engine/Function.hpp"
#include "Engine/Interpolations.hpp"
#include "SFML/Graphics.hpp"
#include "Scripting/ScriptObject.hpp"

namespace ng
{
class Engine;
class Room;
class SoundDefinition;
class SoundTrigger;
class Trigger;
class Entity : public ScriptObject, public sf::Drawable
{
public:
  virtual void update(const sf::Time &elapsed);
  virtual int getZOrder() const = 0;

  void setName(const std::string &name);
  const std::string &getName() const;

  void setVisible(bool isVisible);
  virtual bool isVisible() const;

  void setLit(bool isLit);
  bool isLit() const;

  void setTouchable(bool isTouchable);
  virtual bool isTouchable() const;

  virtual bool isInventoryObject() const = 0;

  void setRenderOffset(const sf::Vector2i &offset);
  sf::Vector2i getRenderOffset() const;

  void setUsePosition(const sf::Vector2f &pos);
  void setPosition(const sf::Vector2f &pos);

  sf::Vector2f getPosition() const;
  sf::Vector2f getRealPosition() const;
  sf::Vector2f getUsePosition() const;

  void setOffset(const sf::Vector2f &offset);
  sf::Vector2f getOffset() const;

  void setRotation(float angle);
  float getRotation() const;

  void setScale(float s);
  virtual float getScale() const;

  void setColor(const sf::Color &color);
  const sf::Color &getColor() const;

  void setTrigger(int triggerNumber, Trigger* pTrigger);
  void trig(int triggerNumber);

  virtual float getVolume() const { return 1.f; }
  virtual void trigSound(const std::string &name);
  virtual void drawForeground(sf::RenderTarget &target, sf::RenderStates states) const;

  virtual Room *getRoom() = 0;
  virtual const Room *getRoom() const = 0;
  virtual void setFps(int fps) = 0;

  virtual HSQOBJECT &getTable() = 0;
  virtual HSQOBJECT &getTable() const = 0;

  SoundTrigger* createSoundTrigger(Engine &engine, const std::vector<SoundDefinition*> &sounds);

  void alphaTo(float destination, sf::Time time, InterpolationMethod method);
  void offsetTo(sf::Vector2f destination, sf::Time time, InterpolationMethod method);
  void moveTo(sf::Vector2f destination, sf::Time time, InterpolationMethod method);
  void rotateTo(float destination, sf::Time time, InterpolationMethod method);
  void scaleTo(float destination, sf::Time time, InterpolationMethod method);

  virtual void stopObjectMotors();

protected:
  sf::Transform getTransform() const;
  sf::Transformable _transform;

private:
  std::map<int, Trigger*> _triggers;
  std::vector<std::unique_ptr<SoundTrigger>> _soundTriggers;
  sf::Vector2f _usePos;
  sf::Vector2f _offset;
  bool _isLit{true};
  bool _isVisible{true};
  bool _isTouchable{true};
  sf::Vector2i _renderOffset;
  std::vector<std::unique_ptr<Function>> _functions;
  sf::Color _color{sf::Color::White};
  std::string _name;
};
} // namespace ng
