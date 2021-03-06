#pragma once
#include "squirrel.h"
#include "Entities/Actor/Actor.hpp"
#include "Engine/Camera.hpp"
#include "Entities/Objects/Animation.hpp"
#include "Dialog/DialogManager.hpp"
#include "Engine/Engine.hpp"
#include "Engine/Function.hpp"
#include "System/Logger.hpp"
#include "Engine/Preferences.hpp"
#include "Room/Room.hpp"
#include "Audio/SoundId.hpp"
#include "Audio/SoundManager.hpp"
#include "Engine/Thread.hpp"
#include "../System/_Util.hpp"

#define SQ_SUSPEND_FLAG -666

namespace ng
{
class _BreakFunction : public Function
{
protected:
    Engine &_engine;
    int _threadId;
    bool _done;

public:
    explicit _BreakFunction(Engine &engine, int id)
        : _engine(engine), _threadId(id), _done(false)
    {
    }

    [[nodiscard]] virtual std::string getName() const
    {
        return "_BreakFunction";
    }

    void operator()(const sf::Time &) override
    {
        if (_done)
            return;

        if (!isElapsed())
            return;

        _done = true;
        auto pThread = ScriptEngine::getThreadFromId(_threadId);
        if (!pThread)
            return;
        pThread->resume();
    }
};

class _BreakHereFunction : public _BreakFunction
{
public:
    explicit _BreakHereFunction(Engine &engine, int id, int numFrames)
        : _BreakFunction(engine, id), _fc(engine.getFrameCounter()), _numFrames(numFrames)
    {
    }

    bool isElapsed() override
    {
        return _engine.getFrameCounter() >= (_fc+_numFrames);
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakHereFunction";
    }

private:
    int _fc;
    int _numFrames;
};

class _BreakWhileAnimatingFunction : public _BreakFunction
{
private:
    std::string _name;
    Actor &_actor;
    CostumeAnimation *_pAnimation;

public:
    _BreakWhileAnimatingFunction(Engine &engine, int id, Actor &actor)
        : _BreakFunction(engine, id), _actor(actor), _pAnimation(actor.getCostume().getAnimation())
    {
        _name = _pAnimation->getName();
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileAnimatingFunction " + _name;
    }

    bool isElapsed() override
    {
        auto pAnim = _actor.getCostume().getAnimation();
        return pAnim != _pAnimation || !_pAnimation->isPlaying();
    }
};

class _BreakWhileAnimatingObjectFunction : public _BreakFunction
{
private:
    std::optional<Animation> &_animation;

public:
    _BreakWhileAnimatingObjectFunction(Engine &engine, int id, Object &object)
        : _BreakFunction(engine, id), _animation(object.getAnimation())
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileAnimatingObjectFunction";
    }

    bool isElapsed() override
    {
        return !_animation.has_value() || !_animation.value().isPlaying();
    }
};

class _BreakWhileWalkingFunction : public _BreakFunction
{
private:
    Actor &_actor;

public:
    explicit _BreakWhileWalkingFunction(Engine &engine, int id, Actor &actor)
        : _BreakFunction(engine, id), _actor(actor)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileWalkingFunction";
    }

    bool isElapsed() override
    {
        return !_actor.isWalking();
    }
};

class _BreakWhileTalkingFunction : public _BreakFunction
{
private:
    Actor &_actor;

public:
    explicit _BreakWhileTalkingFunction(Engine &engine, int id, Actor &actor)
        : _BreakFunction(engine, id), _actor(actor)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileTalkingFunction";
    }

    bool isElapsed() override
    {
        return !_actor.isTalking();
    }
};

class _BreakWhileAnyActorTalkingFunction : public _BreakFunction
{
public:
    explicit _BreakWhileAnyActorTalkingFunction(Engine &engine, int id)
        : _BreakFunction(engine, id)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileAnyActorTalkingFunction";
    }

    bool isElapsed() override
    {
        for(auto&& actor : _engine.getActors())
        {
            if(actor->isTalking())
                return false;
        }
        return true;
    }
};

class _BreakWhileSoundFunction : public _BreakFunction
{
private:
    int _soundId;

public:
    _BreakWhileSoundFunction(Engine &engine, int id, int soundId)
        : _BreakFunction(engine, id), _soundId(soundId)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileSoundFunction";
    }

    bool isElapsed() override
    {
        auto pSoundId = dynamic_cast<SoundId*>(ScriptEngine::getSoundFromId(_soundId));
        return !pSoundId || !pSoundId->isPlaying();
    }
};

class _BreakWhileRunningFunction : public Function
{
private:
    int _currentThreadId, _threadId;
    bool _done;

public:
    _BreakWhileRunningFunction(int currentThreadId, int threadId)
        : _currentThreadId(currentThreadId), _threadId(threadId), _done(false)
    {
    }

    void operator()(const sf::Time &) override
    {
        if (_done)
            return;

        auto pThread = ScriptEngine::getThreadFromId(_threadId);
        if (!pThread || pThread->isStopped())
        {
            auto pCurrentThread = ScriptEngine::getThreadFromId(_currentThreadId);
            if(pCurrentThread)
            {
                pCurrentThread->resume();
            }
            _done = true;
        }
    }

    bool isElapsed() override
    {
        return _done;
    }
};

class _BreakWhileDialogFunction : public _BreakFunction
{
public:
    _BreakWhileDialogFunction(Engine &engine, int id)
        : _BreakFunction(engine, id)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileDialogFunction";
    }

    bool isElapsed() override
    {
        return _engine.getDialogManager().getState() == DialogManagerState::None;
    }
};

class _BreakWhileCutsceneFunction : public _BreakFunction
{
public:
    _BreakWhileCutsceneFunction(Engine &engine, int id)
        : _BreakFunction(engine, id)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileCutsceneFunction";
    }

    bool isElapsed() override
    {
        return !_engine.inCutscene();
    }
};

class _BreakWhileCameraFunction : public _BreakFunction
{
public:
    _BreakWhileCameraFunction(Engine &engine, int id)
        : _BreakFunction(engine, id)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileCameraFunction";
    }

    bool isElapsed() override
    {
        return !_engine.getCamera().isMoving();
    }
};

class _BreakWhileInputOffFunction : public _BreakFunction
{
public:
    _BreakWhileInputOffFunction(Engine &engine, int id)
        : _BreakFunction(engine, id)
    {
    }

    [[nodiscard]] std::string getName() const override
    {
        return "_BreakWhileInputOffFunction";
    }

    bool isElapsed() override
    {
        return _engine.getInputActive();
    }
};

class _BreakTimeFunction : public TimeFunction
{
private:
    int _threadId;

public:
    _BreakTimeFunction(int id, const sf::Time &time)
        : TimeFunction(time), _threadId(id)
    {
    }

    void operator()(const sf::Time &elapsed) override
    {
        if (_done)
            return;
        TimeFunction::operator()(elapsed);
        if (isElapsed())
        {
            _done = true;
            auto pThread = ScriptEngine::getThreadFromId(_threadId);
            if (!pThread)
                return;
            pThread->resume();
        }
    }
};

class _SystemPack : public Pack
{
private:
    static Engine *g_pEngine;
    static ScriptEngine *_pScriptEngine;

private:
    void addTo(ScriptEngine &engine) const override
    {
        g_pEngine = &engine.getEngine();
        _pScriptEngine = &engine;
        engine.registerGlobalFunction(activeController, "activeController");
        engine.registerGlobalFunction(addCallback, "addCallback");
        engine.registerGlobalFunction(addFolder, "addFolder");
        engine.registerGlobalFunction(breakhere, "breakhere");
        engine.registerGlobalFunction(breaktime, "breaktime");
        engine.registerGlobalFunction(breakwhileanimating, "breakwhileanimating");
        engine.registerGlobalFunction(breakwhilecamera, "breakwhilecamera");
        engine.registerGlobalFunction(breakwhilecutscene, "breakwhilecutscene");
        engine.registerGlobalFunction(breakwhiledialog, "breakwhiledialog");
        engine.registerGlobalFunction(breakwhileinputoff, "breakwhileinputoff");
        engine.registerGlobalFunction(breakwhilesound, "breakwhilesound");
        engine.registerGlobalFunction(breakwhilerunning, "breakwhilerunning");
        engine.registerGlobalFunction(breakwhiletalking, "breakwhiletalking");
        engine.registerGlobalFunction(breakwhilewalking, "breakwhilewalking");
        engine.registerGlobalFunction(chr, "chr");
        engine.registerGlobalFunction(cursorPosX, "cursorPosX");
        engine.registerGlobalFunction(cursorPosY, "cursorPosY");
        engine.registerGlobalFunction(exCommand, "exCommand");
        engine.registerGlobalFunction(gameTime, "gameTime");
        engine.registerGlobalFunction(getPrivatePref, "getPrivatePref");
        engine.registerGlobalFunction(getUserPref, "getUserPref");
        engine.registerGlobalFunction(include, "include");
        engine.registerGlobalFunction(inputHUD, "inputHUD");
        engine.registerGlobalFunction(inputOff, "inputOff");
        engine.registerGlobalFunction(inputOn, "inputOn");
        engine.registerGlobalFunction(inputSilentOff, "inputSilentOff");
        engine.registerGlobalFunction(inputState, "inputState");
        engine.registerGlobalFunction(isInputOn, "isInputOn");
        engine.registerGlobalFunction(is_string, "is_string");
        engine.registerGlobalFunction(is_table, "is_table");
        engine.registerGlobalFunction(ord, "ord");
        engine.registerGlobalFunction(inputController, "inputController");
        engine.registerGlobalFunction(inputVerbs, "inputVerbs");
        engine.registerGlobalFunction(logEvent, "logEvent");
        engine.registerGlobalFunction(logInfo, "logInfo");
        engine.registerGlobalFunction(logWarning, "logWarning");
        engine.registerGlobalFunction(microTime, "microTime");
        engine.registerGlobalFunction(moveCursorTo, "moveCursorTo");
        engine.registerGlobalFunction(pushSentence, "pushSentence");
        engine.registerGlobalFunction(removeCallback, "removeCallback");
        engine.registerGlobalFunction(setAmbientLight, "setAmbientLight");
        engine.registerGlobalFunction(setPrivatePref, "setPrivatePref");
        engine.registerGlobalFunction(setUserPref, "setUserPref");
        engine.registerGlobalFunction(startglobalthread, "startglobalthread");
        engine.registerGlobalFunction(startthread, "startthread");
        engine.registerGlobalFunction(stopthread, "stopthread");
        engine.registerGlobalFunction(threadid, "threadid");
        engine.registerGlobalFunction(threadpauseable, "threadpauseable");
    }

    static SQInteger activeController(HSQUIRRELVM v)
    {
        error("TODO: activeController: not implemented");
        // harcode mouse
        sq_pushinteger(v, 1);
        return 1;
    }

    static SQInteger addCallback(HSQUIRRELVM v)
    {
        SQFloat duration;
        if (SQ_FAILED(sq_getfloat(v, 2, &duration)))
        {
            return sq_throwerror(v, _SC("failed to get duration"));
        }
        HSQOBJECT method;
        sq_resetobject(&method);
        if (SQ_FAILED(sq_getstackobj(v, 3, &method)) || !sq_isclosure(method))
        {
            return sq_throwerror(v, _SC("failed to get method"));
        }
        auto callback = std::make_unique<Callback>(v, sf::seconds(duration), method);
        auto id = callback->getId();
        g_pEngine->addCallback(std::move(callback));

        sq_pushinteger(v, id);
        return 1;
    }

    static SQInteger addFolder(HSQUIRRELVM)
    {
        // do nothing
        return 0;
    }

    static SQInteger breakhere(HSQUIRRELVM v)
    {
        SQFloat numFrames;
        if (SQ_FAILED(sq_getfloat(v, 2, &numFrames)))
        {
            return sq_throwerror(v, _SC("failed to get numFrames"));
        }
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();

        g_pEngine->addFunction(std::make_unique<_BreakHereFunction>(*g_pEngine, pThread->getId(), numFrames));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhileanimating(HSQUIRRELVM v)
    {
        auto *pActor = ScriptEngine::getActor(v, 2);
        if (pActor)
        {
            auto pAnim = pActor->getCostume().getAnimation();
            if(!pAnim) return 0;
            
            auto pThread = ScriptEngine::getThreadFromVm(v);
            pThread->suspend();

            g_pEngine->addFunction(std::make_unique<_BreakWhileAnimatingFunction>(*g_pEngine, pThread->getId(), *pActor));
            return SQ_SUSPEND_FLAG;
        }

        auto *pObj = ScriptEngine::getObject(v, 2);
        if (pObj)
        {
            auto pThread = ScriptEngine::getThreadFromVm(v);
            pThread->suspend();

            g_pEngine->addFunction(std::make_unique<_BreakWhileAnimatingObjectFunction>(*g_pEngine, pThread->getId(), *pObj));
            return SQ_SUSPEND_FLAG;
        }
        return sq_throwerror(v, _SC("failed to get actor or object"));
    }

    static SQInteger breakwhilecamera(HSQUIRRELVM v)
    {
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();

        g_pEngine->addFunction(std::make_unique<_BreakWhileCameraFunction>(*g_pEngine, pThread->getId()));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhilecutscene(HSQUIRRELVM v)
    {
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();

        g_pEngine->addFunction(std::make_unique<_BreakWhileCutsceneFunction>(*g_pEngine, pThread->getId()));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhileinputoff(HSQUIRRELVM v)
    {
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();

        g_pEngine->addFunction(std::make_unique<_BreakWhileInputOffFunction>(*g_pEngine, pThread->getId()));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhilesound(HSQUIRRELVM v)
    {
        SoundId *pSound = ScriptEngine::getSound(v, 2);
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();

        g_pEngine->addFunction(std::make_unique<_BreakWhileSoundFunction>(*g_pEngine, pThread->getId(), pSound ? pSound->getId():0));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhiledialog(HSQUIRRELVM v)
    {
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();
        
        g_pEngine->addFunction(std::make_unique<_BreakWhileDialogFunction>(*g_pEngine, pThread->getId()));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhilewalking(HSQUIRRELVM v)
    {
        auto *pActor = ScriptEngine::getActor(v, 2);
        if (!pActor)
        {
            return sq_throwerror(v, _SC("failed to get actor"));
        }
        
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();

        g_pEngine->addFunction(std::make_unique<_BreakWhileWalkingFunction>(*g_pEngine, pThread->getId(), *pActor));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhiletalking(HSQUIRRELVM v)
    {
        if(sq_gettop(v) == 2)
        {
            auto *pActor = ScriptEngine::getActor(v, 2);
            if (!pActor)
            {
                return sq_throwerror(v, _SC("failed to get actor"));
            }
            auto pThread = ScriptEngine::getThreadFromVm(v);
            pThread->suspend();

            g_pEngine->addFunction(std::make_unique<_BreakWhileTalkingFunction>(*g_pEngine, pThread->getId(), *pActor));
            return SQ_SUSPEND_FLAG;
        }
       
        auto pThread = ScriptEngine::getThreadFromVm(v);
        pThread->suspend();
        
        g_pEngine->addFunction(std::make_unique<_BreakWhileAnyActorTalkingFunction>(*g_pEngine, pThread->getId()));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger breakwhilerunning(HSQUIRRELVM v)
    {
        SQInteger id = 0;
        if (sq_gettype(v, 2) == OT_INTEGER)
        {
            sq_getinteger(v, 2, &id);
        }

        if(ResourceManager::isThread(id))
        {
            auto pCurrentThread = ScriptEngine::getThreadFromVm(v);
            if(!pCurrentThread)
            {
                return sq_throwerror(v, "Current thread should be created with startthread");
            }
            
            auto pThread = ScriptEngine::getThreadFromId(id);
            if(!pThread) return 0;
            
            pCurrentThread->suspend();
            g_pEngine->addFunction(std::make_unique<_BreakWhileRunningFunction>(pCurrentThread->getId(), id));
            return SQ_SUSPEND_FLAG;
        }
        return breakwhilesound(v);
    }

    static SQInteger chr(HSQUIRRELVM v)
    {
        SQInteger number;
        if (SQ_FAILED(sq_getinteger(v, 2, &number)))
        {
            return sq_throwerror(v, "Failed to get number");
        }
        auto character = (char)number;
        char s[]{character,'\0'};
        sq_pushstring(v, s, -1);
        return 1;
    }

    static SQInteger cursorPosX(HSQUIRRELVM v)
    {
        auto pos = g_pEngine->getMousePositionInRoom();
        sq_pushinteger(v, static_cast<SQInteger>(pos.x));
        return 1;
    }

    static SQInteger cursorPosY(HSQUIRRELVM v)
    {
        auto pos = g_pEngine->getMousePositionInRoom();
        sq_pushinteger(v, static_cast<SQInteger>(pos.y));
        return 1;
    }

    static SQInteger exCommand(HSQUIRRELVM)
    {
        error("TODO: exCommand: not implemented");
        return 0;
    }

    static SQInteger gameTime(HSQUIRRELVM v)
    {
        sq_pushfloat(v, g_pEngine->getTime().asSeconds());
        return 1;
    }

    static SQInteger logEvent(HSQUIRRELVM v)
    {
        auto numArgs = sq_gettop(v);
        const SQChar* event = nullptr;
        if (SQ_SUCCEEDED(sq_getstring(v, 2, &event)))
        {
            info(event);
        }
        if(numArgs == 3)
        {
            if (SQ_SUCCEEDED(sq_getstring(v, 3, &event)))
            {
                info(event);
            }
        }
        return 0;
    }

    static SQInteger logInfo(HSQUIRRELVM v)
    {
        const SQChar* msg = nullptr;
        if (SQ_SUCCEEDED(sq_getstring(v, 2, &msg)))
        {
            info(msg);
        }
        return 0;
    }

    static SQInteger logWarning(HSQUIRRELVM v)
    {
        const SQChar* msg = nullptr;
        if (SQ_SUCCEEDED(sq_getstring(v, 2, &msg)))
        {
            error(msg);
        }
        return 0;
    }

    static SQInteger microTime(HSQUIRRELVM v)
    {
        sq_pushfloat(v, g_pEngine->getTime().asMilliseconds());
        return 1;
    }

    static SQInteger moveCursorTo(HSQUIRRELVM v)
    {
        SQInteger x;
        if (SQ_FAILED(sq_getinteger(v, 2, &x)))
        {
            return sq_throwerror(v, _SC("Failed to get x"));
        }
        SQInteger y;
        if (SQ_FAILED(sq_getinteger(v, 3, &y)))
        {
            return sq_throwerror(v, _SC("Failed to get y"));
        }
        SQFloat t;
        if (SQ_FAILED(sq_getfloat(v, 4, &t)))
        {
            return sq_throwerror(v, _SC("Failed to get time"));
        }

        // WIP need to be check
        auto pos = g_pEngine->getWindow().mapCoordsToPixel(sf::Vector2f(x,y)-g_pEngine->getCamera().getAt());
        sf::Mouse::setPosition(pos, g_pEngine->getWindow());
        error("moveCursorTo not implemented");
        return 0;
    }

    static SQInteger pushSentence(HSQUIRRELVM v)
    {
        auto numArgs = sq_gettop(v);
        SQInteger id;
        if (SQ_FAILED(sq_getinteger(v, 2, &id)))
        {
            return sq_throwerror(v, _SC("Failed to get verb id"));
        }
        
        if(id == VerbConstants::VERB_DIALOG)
        {
            SQInteger choice;
            if (SQ_FAILED(sq_getinteger(v, 3, &choice)))
            {
                return sq_throwerror(v, _SC("Failed to get choice"));
            }
            g_pEngine->getDialogManager().choose(choice);
            return 0;
        }

        Entity* pObj1{nullptr};
        Entity* pObj2{nullptr};
        if(numArgs > 2)
        {
            pObj1 = ScriptEngine::getEntity(v, 3);
            if (!pObj1)
            {
                return sq_throwerror(v, _SC("Failed to get obj1"));
            }
        }
        if(numArgs > 3)
        {
            pObj2 = ScriptEngine::getEntity(v, 4);
            if (!pObj2)
            {
                return sq_throwerror(v, _SC("Failed to get obj2"));
            }
        }
        g_pEngine->pushSentence(static_cast<int>(id), pObj1, pObj2);
        return 0;
    }

    static SQInteger stopthread(HSQUIRRELVM v)
    {
        auto type = sq_gettype(v, 2);
        if(type == OT_NULL) return 0;
        
        SQInteger id;
        if (SQ_FAILED(sq_getinteger(v, 2, &id)))
        {
            return sq_throwerror(v, _SC("Failed to get thread id"));
        }

        auto pThread = ScriptEngine::getThreadFromId(id);
        if(!pThread) return 0;

        trace("stopthread {}", id);
        pThread->stop();

        sq_pushinteger(v, 0);
        return 1;
    }

    static SQInteger startglobalthread(HSQUIRRELVM v)
    {
        return startthread(v, true);
    }

    static SQInteger startthread(HSQUIRRELVM v)
    {
        return startthread(v, false);
    }

    static SQInteger startthread(HSQUIRRELVM v, bool global)
    {
        SQInteger size = sq_gettop(v);

        HSQOBJECT env_obj;
        sq_resetobject(&env_obj);
        if (SQ_FAILED(sq_getstackobj(v, 1, &env_obj)))
        {
            return sq_throwerror(v, _SC("Couldn't get environment from stack"));
        }

        auto vm = g_pEngine->getVm();
        // create thread and store it on the stack
        sq_newthread(vm, 1024);
        HSQOBJECT thread_obj;
        sq_resetobject(&thread_obj);
        if (SQ_FAILED(sq_getstackobj(vm, -1, &thread_obj)))
        {
            return sq_throwerror(v, _SC("Couldn't get coroutine thread from stack"));
        }

        std::vector<HSQOBJECT> args;
        for (auto i = 0; i < size - 2; i++)
        {
            HSQOBJECT arg;
            sq_resetobject(&arg);
            if (SQ_FAILED(sq_getstackobj(v, 3 + i, &arg)))
            {
                return sq_throwerror(v, _SC("Couldn't get coroutine args from stack"));
            }
            args.push_back(arg);
        }

        // get the closure
        HSQOBJECT closureObj;
        sq_resetobject(&closureObj);
        if (SQ_FAILED(sq_getstackobj(v, 2, &closureObj)))
        {
            return sq_throwerror(v, _SC("Couldn't get coroutine thread from stack"));
        }

        const SQChar *name = nullptr;
        if (SQ_SUCCEEDED(sq_getclosurename(v, 2)))
        {
            sq_getstring(v, -1, &name);
        }

        auto pUniquethread = std::make_unique<Thread>(global, vm, thread_obj, env_obj, closureObj, args);
        auto pThread = pUniquethread.get();
        trace("start thread ({}): {}", (name ? name : "anonymous"), pThread->getId());

        g_pEngine->addThread(std::move(pUniquethread));

        // call the closure in the thread
        if (!pThread->call())
        {
            return sq_throwerror(v, _SC("call failed"));
        }

        sq_pushinteger(v, pThread->getId());
        return 1;
    }

    static SQInteger breaktime(HSQUIRRELVM v)
    {
        SQFloat time = 0;
        if (SQ_FAILED(sq_getfloat(v, 2, &time)))
        {
            return sq_throwerror(v, _SC("failed to get time"));
        }

        auto pThread = ScriptEngine::getThreadFromVm(v);
        if(!pThread)
        {
            return sq_throwerror(v, _SC("failed to get thread"));
        }

        pThread->suspend();

        g_pEngine->addFunction(std::make_unique<_BreakTimeFunction>(pThread->getId(), sf::seconds(time)));
        return SQ_SUSPEND_FLAG;
    }

    static SQInteger setPrivatePref(HSQUIRRELVM v)
    {
        _setPref(v, 
            [](auto key,auto value) { return g_pEngine->getPreferences().setPrivatePreference(key,value); },
            [](auto key) { return g_pEngine->getPreferences().removePrivatePreference(key); });
        return 0;
    }

    static SQInteger getPrivatePref(HSQUIRRELVM v)
    {
        return _getPref(v, [](auto name, auto value){ return g_pEngine->getPreferences().getPrivatePreference(name,value);});
    }

    static SQInteger getUserPref(HSQUIRRELVM v)
    {
        return _getPref(v, [](auto name, auto value){ return g_pEngine->getPreferences().getUserPreference(name,value);});
    }

    static SQInteger _getPref(HSQUIRRELVM v, const std::function<GGPackValue(const std::string &name, GGPackValue value)>& func)
    {
        const SQChar *key;
        if (SQ_FAILED(sq_getstring(v, 2, &key)))
        {
            return sq_throwerror(v, _SC("failed to get key"));
        }
        auto numArgs = sq_gettop(v) - 1;
        GGPackValue defaultValue;
        if (numArgs > 1)
        {
            auto type = sq_gettype(v, 3);
            if (type == SQObjectType::OT_STRING)
            {
                const SQChar *str = nullptr;
                sq_getstring(v, 3, &str);
                std::string strValue = str;
                defaultValue.type = 4;
                defaultValue.string_value = strValue;
            }
            else if (type == SQObjectType::OT_INTEGER)
            {
                SQInteger integer;
                sq_getinteger(v, 3, &integer);
                defaultValue.type = 5;
                defaultValue.int_value = integer;
            }
            else if (type == SQObjectType::OT_BOOL)
            {
                SQBool b;
                sq_getbool(v, 3, &b);
                defaultValue.type = 5;
                defaultValue.int_value = b ? 1 : 0;
            }
            else if (type == SQObjectType::OT_FLOAT)
            {
                SQFloat fl;
                sq_getfloat(v, 3, &fl);
                defaultValue.type = 6;
                defaultValue.double_value = fl;
            }
        }

        auto value = func(key, defaultValue);
        if (value.isString())
        {
            sq_pushstring(v, value.string_value.data(), -1);
        }
        else if (value.isInteger())
        {
            sq_pushinteger(v, value.int_value);
        }
        else if (value.isDouble())
        {
            sq_pushfloat(v, value.double_value);
        }
        else
        {
            sq_pushnull(v);
        }

        return 1;
    }

    static SQInteger removeCallback(HSQUIRRELVM v)
    {
        SQInteger id = 0;
        if (SQ_FAILED(sq_getinteger(v, 2, &id)))
        {
            return sq_throwerror(v, _SC("failed to get callback"));
        }
        g_pEngine->removeCallback(id);
        return 0;
    }

    static SQInteger setAmbientLight(HSQUIRRELVM v)
    {
        SQInteger c = 0;
        if (SQ_FAILED(sq_getinteger(v, 2, &c)))
        {
            return sq_throwerror(v, _SC("failed to get color"));
        }
        auto color = _fromRgb(c);
        g_pEngine->getRoom()->setAmbientLight(color);
        return 0;
    }

    static SQInteger setUserPref(HSQUIRRELVM v)
    {
        _setPref(v, 
            [](auto key,auto value) { return g_pEngine->getPreferences().setUserPreference(key,value); },
            [](auto key) { return g_pEngine->getPreferences().removeUserPreference(key); });
        return 0;
    }

    static SQInteger _setPref(HSQUIRRELVM v, const std::function<void(const std::string&, GGPackValue)>& setPref, const std::function<void(const std::string&)>& removePref)
    {
        const SQChar *key;
        if (SQ_FAILED(sq_getstring(v, 2, &key)))
        {
            return sq_throwerror(v, _SC("failed to get key"));
        }
        auto type = sq_gettype(v, 3);
        if (type == SQObjectType::OT_STRING)
        {
            const SQChar *str = nullptr;
            sq_getstring(v, 3, &str);
            std::string strValue = str;
            setPref(key, Preferences::toGGPackValue(strValue));
            return 0;
        }
        if (type == SQObjectType::OT_INTEGER)
        {
            SQInteger integer;
            sq_getinteger(v, 3, &integer);
            setPref(key, Preferences::toGGPackValue(static_cast<int>(integer)));
            return 0;
        }
        if (type == SQObjectType::OT_BOOL)
        {
            SQBool b;
            sq_getbool(v, 3, &b);
            setPref(key, Preferences::toGGPackValue(b?true:false));
            return 0;
        }
        if (type == SQObjectType::OT_FLOAT)
        {
            SQFloat fl;
            sq_getfloat(v, 3, &fl);
            setPref(key, Preferences::toGGPackValue(fl));
            return 0;
        }

        removePref(key);

        return 0;
    }

    static SQInteger include(HSQUIRRELVM v)
    {
        const SQChar *filename = nullptr;
        if (SQ_FAILED(sq_getstring(v, 2, &filename)))
        {
            return sq_throwerror(v, "failed to get filename");
        }
        trace("include {}", filename);
        _pScriptEngine->executeNutScript(filename);
        return 0;
    }

    static SQInteger inputHUD(HSQUIRRELVM v)
    {
        SQInteger on;
        if (SQ_FAILED(sq_getinteger(v, 2, &on)))
        {
            return sq_throwerror(v, _SC("failed to get on"));
        }
        g_pEngine->setInputHUD(on);
        return 0;
    }

    static SQInteger inputOff(HSQUIRRELVM)
    {
        g_pEngine->setInputActive(false);
        return 0;
    }

    static SQInteger inputOn(HSQUIRRELVM)
    {
        g_pEngine->setInputActive(true);
        return 0;
    }

    static SQInteger inputSilentOff(HSQUIRRELVM)
    {
        g_pEngine->inputSilentOff();
        return 0;
    }

    static SQInteger isInputOn(HSQUIRRELVM v)
    {
        bool isActive = g_pEngine->getInputActive();
        sq_push(v, isActive ? SQTrue : SQFalse);
        return 1;
    }

    static SQInteger inputState(HSQUIRRELVM v)
    {
        auto numArgs = sq_gettop(v);
        if(numArgs == 1)
        {
            auto state = g_pEngine->getInputState();
            sq_pushinteger(v, state);
            return 1;
        }
        else if(numArgs == 2)
        {
             SQInteger state;
            if (SQ_FAILED(sq_getinteger(v, 2, &state)))
            {
                return sq_throwerror(v, _SC("failed to get state"));
            }
            g_pEngine->setInputState(state);
            return 0;
        }
        error("TODO: inputState: not implemented");
        return 0;
    }

    static SQInteger inputController(HSQUIRRELVM)
    {
        error("TODO: inputController: not implemented");
        return 0;
    }

    static SQInteger inputVerbs(HSQUIRRELVM v)
    {
        SQInteger on;
        if (SQ_FAILED(sq_getinteger(v, 2, &on)))
        {
            return sq_throwerror(v, _SC("failed to get isActive"));
        }
        g_pEngine->setInputVerbs(on);
        return 1;
    }

    static SQInteger is_table(HSQUIRRELVM v)
    {
        sq_pushbool(v, sq_gettype(v, 2) == OT_TABLE ? SQTrue : SQFalse);
        return 1;
    }

    static SQInteger ord(HSQUIRRELVM v)
    {
        const SQChar *letter;
        if (SQ_FAILED(sq_getstring(v, 2, &letter)))
        {
            return sq_throwerror(v, "Failed to get letter");
        }
        sq_pushinteger(v, (SQInteger)letter[0]);
        return 1;
    }

    static SQInteger is_string(HSQUIRRELVM v)
    {
        sq_pushbool(v, sq_gettype(v, 2) == OT_STRING ? SQTrue : SQFalse);
        return 1;
    }

    static SQInteger threadid(HSQUIRRELVM v)
    {
        auto pThread = ScriptEngine::getThreadFromVm(v);
        sq_pushinteger(v, pThread ? pThread->getId() : 0);
        return 1;
    }

    static SQInteger threadpauseable(HSQUIRRELVM v)
    {
        SQInteger threadId = 0;
        if (SQ_FAILED(sq_getinteger(v, 2, &threadId)))
        {
            return sq_throwerror(v, _SC("failed to get threadId"));
        }
        auto pThread = ScriptEngine::getThreadFromId(threadId);
        if (!pThread)
        {
            return 0;
        }
        SQInteger pauseable = 0;
        if (SQ_FAILED(sq_getinteger(v, 3, &pauseable)))
        {
            return sq_throwerror(v, _SC("failed to get pauseable"));
        }
        pThread->setPauseable(pauseable != 0);
        return 0;
    }
};

Engine *_SystemPack::g_pEngine = nullptr;
ScriptEngine *_SystemPack::_pScriptEngine = nullptr;

} // namespace ng
