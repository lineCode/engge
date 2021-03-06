#pragma once

#include "../System/_Util.hpp"
#include "Engine/Engine.hpp"
#include "System/Logger.hpp"
#include "Engine/Sentence.hpp"
#include "Scripting/ScriptEngine.hpp"
#include "squirrel.h"

namespace ng
{

class _ActorWalk : public Function
{
 public:
    _ActorWalk(Actor &actor, const Entity *pEntity) : _actor(actor)
    {
        auto pos = pEntity->getRealPosition();
        auto roomHeight = pEntity->getRoom()->getRoomSize().y;
        auto usePos = pEntity->getUsePosition();
        pos.x += usePos.x;
        pos.y += usePos.y - roomHeight;
        auto facing = getFacing(pEntity);
        _actor.walkTo(pos, facing);
    }

 private:
    static Facing getFacing(const Entity *pEntity)
    {
        const auto pActor = dynamic_cast<const Actor *>(pEntity);
        if (pActor)
        {
            return getOppositeFacing(pActor->getCostume().getFacing());
        }
        const auto pObj = dynamic_cast<const Object *>(pEntity);
        return _toFacing(pObj->getUseDirection());
    }

 private:
    bool isElapsed() override { return !_actor.isWalking(); }

 private:
    Actor &_actor;
};

class _PostWalk : public Function
{
 public:
    _PostWalk(Sentence& sentence, Entity *pObject, Entity *pObject2, int verb)
        : _sentence(sentence), _pObject(pObject), _pObject2(pObject2), _verb(verb)
    {
    }

    bool isElapsed() override { return _done; }

    void operator()(const sf::Time &) override
    {
        auto *pObj = dynamic_cast<Object *>(_pObject);
        auto functionName = pObj ? "objectPostWalk" : "actorPostWalk";
        bool handled = false;
        ScriptEngine::callFunc(handled, _pObject, functionName, _verb, _pObject, _pObject2);
        if(handled)
        {
            _sentence.stop();
        }
        _done = true;
    }

 private:
    Sentence& _sentence;
    Entity *_pObject{nullptr};
    Entity *_pObject2{nullptr};
    int _verb{0};
    bool _done{false};
};

class _SetDefaultVerb : public Function
{
 public:
    explicit _SetDefaultVerb(Engine &engine) : _engine(engine) {}

    bool isElapsed() override { return _done; }

    void operator()(const sf::Time &) override
    {
        if (_done)
            return;

        _done = true;
        _engine.setDefaultVerb();
    }

 private:
    Engine &_engine;
    bool _done{false};
};

class _ReachAnim : public Function
{
  public:
    _ReachAnim(Actor &actor, Entity* obj)
        : _actor(actor), _pObject(obj)
    {
        int flags = 0;
        ScriptEngine::get(_pObject, "flags", flags);

        if ((flags & ObjectFlagConstants::REACH_HIGH) == ObjectFlagConstants::REACH_HIGH)
        {
            _animName = "reach_high";
        }
        else if ((flags & ObjectFlagConstants::REACH_MED) == ObjectFlagConstants::REACH_MED)
        {
            _animName = "reach_med";
        }
        else if ((flags & ObjectFlagConstants::REACH_LOW) == ObjectFlagConstants::REACH_LOW)
        {
            _animName = "reach_low";
        }

        if(_animName.empty()) {
            _state = 2;
        }
    }

  private:
    bool isElapsed() override { return _state == 3; }

    void playAnim(const std::string &name)
    {
        trace("Play anim {}", name);
        _actor.getCostume().setState(name);
        _pAnim = _actor.getCostume().getAnimation();
        if (_pAnim)
        {
            _pAnim->play(false);
        }
    }

    void operator()(const sf::Time & elapsed) override
    {
        switch (_state)
        {
            case 0:
                playAnim(_animName);
                _state = 1;
                break;
            case 1:
                _elapsed+=elapsed;
                if (_elapsed.asSeconds()>0.330)
                    _state = 2;
                break;
            case 2:
                playAnim("stand");
                _state = 3;
                break;
        }
    }

  private:
    int32_t _state{0};
    Actor &_actor;
    Entity* _pObject;
    std::string _animName;
    CostumeAnimation *_pAnim{nullptr};
    sf::Time _elapsed;
};

class _GiveFunction : public Function
{
 public:
    _GiveFunction(Actor* pActor, Entity* pObject, Sentence* pSentence)
        : _pActor(pActor), _pObject(pObject), _pSentence(pSentence)
    {
    }

 private:
    bool isElapsed() override { return _done; }

    void operator()(const sf::Time &) override
    {
        bool result = false;
        if(ScriptEngine::callFunc(result, _pActor, "verbGive", _pObject) && result) {
            _pSentence->stop();
        }
        _done = true;
    }

 private:
    Actor* _pActor{nullptr}; 
    Entity* _pObject{nullptr};
    Sentence* _pSentence{nullptr};
    bool _done{false};
};

class _TalkToFunction : public Function
{
 public:
    _TalkToFunction(Actor* pActor, Sentence* pSentence)
        : _pActor(pActor), _pSentence(pSentence)
    {
    }

 private:
    bool isElapsed() override { return _done; }

    void operator()(const sf::Time &) override
    {
        if(ScriptEngine::call(_pActor, "verbTalkTo")) {
            _pSentence->stop();
        }
        _done = true;
    }

 private:
    Actor* _pActor{nullptr}; 
    Sentence* _pSentence{nullptr};
    bool _done{false};
};

class _VerbExecute : public Function
{
 public:
    _VerbExecute(Engine &engine, Actor &actor, Entity &object, Entity* pObject2, const Verb *pVerb)
        : _engine(engine), _pVerb(pVerb), _object(object), _pObject2(pObject2), _actor(actor)
    {
    }

 private:
    bool isElapsed() override { return _done; }

    void operator()(const sf::Time &) override
    {
        _done = true;

        bool success;
        if(_pObject2)
        {
            success = ScriptEngine::call(&_object, _pVerb->func.data(), _pObject2);
        }
        else
        {
            success = ScriptEngine::call(&_object, _pVerb->func.data());
        }

        if(success)
        {
            onPickup();
            return;
        }

        if(_pVerb->id == VerbConstants::VERB_GIVE)
        {
            ScriptEngine::call("objectGive", &_object, &_actor, _pObject2);

            auto* pObject = dynamic_cast<Object*>(&_object);
            auto* pActor2 = dynamic_cast<Actor*>(_pObject2);
            _actor.giveTo(pObject, pActor2);
            return;
        }

        if (callVerbDefault(&_object))
            return;

        if (callDefaultObjectVerb())
            return;
    }

    void onPickup()
    {
        if(_pVerb->id != VerbConstants::VERB_PICKUP)
            return;

        ScriptEngine::call("onPickup", &_actor, &_object);
    }

    bool callDefaultObjectVerb()
    {
        auto &obj = _engine.getDefaultObject();
        auto pActor = _engine.getCurrentActor();

        return ScriptEngine::call(obj, _pVerb->func.data(), pActor, &_object);
    }

    bool callVerbDefault(Entity* pEntity)
    {
        return ScriptEngine::call(pEntity, "verbDefault");
    }

 private:
    Engine &_engine;
    const Verb *_pVerb{nullptr};
    Entity &_object;
    Entity* _pObject2{nullptr};
    Actor& _actor;
    bool _done{false};
};

class _DefaultVerbExecute : public VerbExecute
{
 public:
    _DefaultVerbExecute(HSQUIRRELVM vm, Engine &engine) : _vm(vm), _engine(engine) {}

 private:
    void execute(const Verb *pVerb, Entity *pObject1, Entity *pObject2) override
    {
        auto obj = pObject1->getTable();
        getVerb(pObject1, pVerb);
        if (!pVerb)
            return;

        if (pVerb->id == VerbConstants::VERB_USE && !pObject2 && useFlags(pObject1))
            return;

        if(pVerb->id == VerbConstants::VERB_GIVE && !pObject2)
        {
            _engine.setUseFlag(UseFlag::GiveTo, pObject1);
            return;
        }

        if (callObjectOrActorPreWalk(pVerb->id, pObject1, pObject2))
            return;

        auto pActor = _engine.getCurrentActor();
        if (!pActor)
            return;
        Entity* pObj = pObject2 ? pObject2 : pObject1;
        auto sentence = std::make_unique<Sentence>();
        if ((pVerb->id != VerbConstants::VERB_LOOKAT || !isFarLook(obj)) &&
            !pObj->isInventoryObject())
        {
            auto walk = std::make_unique<_ActorWalk>(*pActor, pObj);
            sentence->push_back(std::move(walk));
        }
        auto postWalk = std::make_unique<_PostWalk>(*sentence, pObject1, pObject2, pVerb->id);
        sentence->push_back(std::move(postWalk));

        if(pVerb->id == VerbConstants::VERB_GIVE) 
        {
            auto func = std::make_unique<_GiveFunction>(pActor,pObject1,sentence.get());
            sentence->push_back(std::move(func));
        }
        else if(pVerb->id == VerbConstants::VERB_TALKTO) 
        {
            auto func = std::make_unique<_TalkToFunction>(pActor,sentence.get());
            sentence->push_back(std::move(func));
        }

        if(pVerb->id == VerbConstants::VERB_PICKUP || pVerb->id == VerbConstants::VERB_OPEN || pVerb->id == VerbConstants::VERB_CLOSE ||
           pVerb->id == VerbConstants::VERB_PUSH || pVerb->id == VerbConstants::VERB_PULL || pVerb->id == VerbConstants::VERB_USE)
        {
            if(ScriptEngine::exists(pObject1, pVerb->func.data())) {
                auto reach = std::make_unique<_ReachAnim>(*pActor, pObject1);
                sentence->push_back(std::move(reach));
            }
        }
        auto verb = std::make_unique<_VerbExecute>(_engine, *pActor, *pObject1, pObject2, pVerb);
        sentence->push_back(std::move(verb));
        auto setDefaultVerb = std::make_unique<_SetDefaultVerb>(_engine);
        sentence->push_back(std::move(setDefaultVerb));
        _engine.setSentence(std::move(sentence));
    }

    bool isFarLook(HSQOBJECT obj)
    {
        auto flags = getFlags(obj);
        return ((flags & 0x8) == 0x8);
    }

    bool useFlags(Entity *pObject)
    {
        HSQOBJECT obj = pObject->getTable();
        SQInteger flags = 0;
        sq_pushobject(_vm, obj);
        sq_pushstring(_vm, _SC("flags"), -1);
        if (SQ_SUCCEEDED(sq_get(_vm, -2)))
        {
            sq_getinteger(_vm, -1, &flags);
            UseFlag useFlag;
            switch (flags)
            {
                case ObjectFlagConstants::USE_WITH:
                    useFlag = UseFlag::UseWith;
                    break;
                case ObjectFlagConstants::USE_ON:
                    useFlag = UseFlag::UseOn;
                    break;
                case ObjectFlagConstants::USE_IN:
                    useFlag = UseFlag::UseIn;
                    break;
                default:
                    return false;
            }
            _engine.setUseFlag(useFlag, pObject);
            return true;
        }
        return false;
    }

    int32_t getFlags(HSQOBJECT obj)
    {
        SQInteger flags = 0;
        sq_pushobject(_vm, obj);
        sq_pushstring(_vm, _SC("flags"), -1);
        if (SQ_SUCCEEDED(sq_get(_vm, -2)))
        {
            sq_getinteger(_vm, -1, &flags);
        }
        sq_pop(_vm, 2);
        return flags;
    }

    static bool callObjectOrActorPreWalk(int verb, Entity *pObj1, Entity *pObj2)
    {
        auto *pObj = dynamic_cast<Object *>(pObj1);
        auto functionName = pObj ? "objectPreWalk" : "actorPreWalk";
        bool handled = false;
        ScriptEngine::callFunc(handled, pObj1, functionName, verb, pObj1, pObj2);
        return handled;
    }

    void getVerb(Entity* pObj, const Verb *&pVerb)
    {
        int verb;
        if (pVerb)
        {
            verb = pVerb->id;
            pVerb = _engine.getVerb(verb);
            return;
        }
        auto defaultVerb = getDefaultVerb(pObj);
        if (!defaultVerb)
            return;
        verb = defaultVerb;
        pVerb = _engine.getVerb(verb);
    }

    SQInteger getDefaultVerb(Entity* pObj)
    {
        sq_pushobject(_vm, pObj->getTable());
        sq_pushstring(_vm, _SC("defaultVerb"), -1);

        if (SQ_SUCCEEDED(sq_get(_vm, -2)))
        {
            SQInteger defaultVerb = 0;
            sq_getinteger(_vm, -1, &defaultVerb);
            trace("defaultVerb: {}", defaultVerb);
            sq_pop(_vm, 2);
            return defaultVerb;
        }
        sq_pop(_vm, 1);

        return 2;
    }

 private:
    HSQUIRRELVM _vm;
    Engine &_engine;
};
} // namespace ng
