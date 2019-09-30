#include "squirrel.h"
#include "../extlibs/squirrel/squirrel/sqpcheader.h"
#include "../extlibs/squirrel/squirrel/sqvm.h"
#include "../extlibs/squirrel/squirrel/sqstring.h"
#include "../extlibs/squirrel/squirrel/sqtable.h"
#include "../extlibs/squirrel/squirrel/sqarray.h"
#include "../extlibs/squirrel/squirrel/sqfuncproto.h"
#include "../extlibs/squirrel/squirrel/sqclosure.h"
#include "../extlibs/squirrel/squirrel/squserdata.h"
#include "../extlibs/squirrel/squirrel/sqcompiler.h"
#include "../extlibs/squirrel/squirrel/sqfuncstate.h"
#include "../extlibs/squirrel/squirrel/sqclass.h"
#include "Object.h"
#include "_Util.h"
#include "imgui-SFML.h"
#include "imgui.h"

namespace ng
{
class _DebugTools
{
  public:
    explicit _DebugTools(Engine &engine) : _engine(engine) {}

    void render()
    {
        static auto stackGetter = [](void *vec, int idx, const char **out_text) {
            auto &vector = *static_cast<std::vector<std::string> *>(vec);
            if (idx < 0 || idx >= static_cast<int>(vector.size()))
            {
                return false;
            }
            *out_text = vector.at(idx).c_str();
            return true;
        };

        ImGui::Begin("Debug");
        std::stringstream s;
        s << "Stack: " << sq_gettop(_engine.getVm());
        std::vector<std::string> stack;
        getStack(stack);
        ImGui::Combo("", &_selectedStack, stackGetter, static_cast<void *>(&stack), stack.size());
        ImGui::TextUnformatted(s.str().c_str());
        ImGui::Text("In cutscene: %s", _engine.inCutscene() ? "yes" : "no");
        ImGui::Text("In dialog: %s", _engine.getDialogManager().isActive() ? "yes" : "no");
        ImGui::Separator();
        ImGui::Text("Cursor visible: %s", _engine.isCursorVisible() ? "yes" : "no");
        auto inputActive = _engine.getInputActive();
        if(ImGui::Checkbox("Input active", &inputActive))
        {
            _engine.setInputActive(inputActive);
        }
        auto inputVerbs = _engine.getInputVerbs();
        if(ImGui::Checkbox("Input verbs", &inputVerbs))
        {
            _engine.setInputVerbs(inputVerbs);
        }
        auto inputHUD = _engine.getInputHUD();
        if(ImGui::Checkbox("Input HUD", &inputHUD))
        {
            _engine.setInputHUD(inputHUD);
        }
        ImGui::Separator();
        ImGui::Checkbox("Actors", &_showActors);
        ImGui::Checkbox("Objects", &_showObjects);
        ImGui::Checkbox("Rooms", &_showRooms);
        ImGui::Separator();
        auto &rooms = _engine.getRooms();
        int currentRoom = 0;
        for (int i = 0; i < rooms.size(); i++)
        {
            if (rooms[i].get() == _engine.getRoom())
            {
                currentRoom = i;
                break;
            }
        }
        if (ImGui::Combo("Room", &currentRoom, roomGetter, static_cast<void *>(&rooms), rooms.size()))
        {
            _engine.setRoom(rooms[currentRoom].get());
        }
        ImGui::End();

        if (_showActors)
        {
            showActors();
        }

        if (_showObjects)
        {
            showObjects();
        }

        if (_showRooms)
        {
            showRooms();
        }
    }

  private:
    void getStack(std::vector<std::string>& stack)
    {
        HSQOBJECT obj;
        auto size = sq_gettop(_engine.getVm());
        for(size_t i=1;i<=size;++i){
            auto type = sq_gettype(_engine.getVm(),-i);
            sq_getstackobj(_engine.getVm(),-i,&obj);
            std::ostringstream s;
            s << "#" << i << ": ";
            switch (type)
            {
            case OT_NULL:
                s << "null";
                break;
            case OT_INTEGER:
                s << sq_objtointeger(&obj);
                break;
            case OT_FLOAT:
                s << sq_objtofloat(&obj);
                break;
            case OT_BOOL:
                s << (sq_objtobool(&obj)==SQTrue?"true":"false");
                break;
            case OT_USERPOINTER:
            {
                s << "userpointer";
                auto ptr = _userpointer(obj);
                auto p = (ScriptObject*)ptr;
                break;
            }
            case OT_STRING:
                s << sq_objtostring(&obj);
                break;
            case OT_TABLE:
                s << "table";
                break;
            case OT_ARRAY:
                s << "array";
                break;
            case OT_CLOSURE:
            {
                s << "closure: ";
                auto pName = _closure(obj)->_function->_name;
                s << (pName._type!=OT_NULL?_stringval(pName):"null");
                break;
            }
            case OT_NATIVECLOSURE:
                s << "native closure";
                break;
            case OT_GENERATOR:
                s << "generator";
                break;
            case OT_USERDATA:
                s << "user data";
                break;
            case OT_THREAD:
                s << "thread";
                break;
            case OT_INSTANCE:
                s << "instance";
                break;
            case OT_WEAKREF:
                s << "weak ref";
                break;
            default:
                s << "?";
                break;
            }
            stack.push_back(s.str());
        }
    }
    void showActors()
    {
        static auto actor_getter = [](void *vec, int idx, const char **out_text) {
            auto &vector = *static_cast<std::vector<std::unique_ptr<Actor>> *>(vec);
            if (idx < 0 || idx >= static_cast<int>(vector.size()))
            {
                return false;
            }
            *out_text = vector.at(idx)->getName().c_str();
            return true;
        };

        ImGui::Begin("Actors", &_showActors);
        auto &actors = _engine.getActors();
        ImGui::Combo("", &_selectedActor, actor_getter, static_cast<void *>(&actors), actors.size());
        auto &actor = actors[_selectedActor];
        auto isVisible = actor->isVisible();
        if (ImGui::Checkbox("Visible", &isVisible))
        {
            actor->setVisible(isVisible);
        }
        auto isTouchable = actor->isTouchable();
        if (ImGui::Checkbox("Touchable", &isTouchable))
        {
            actor->setTouchable(isTouchable);
        }
        auto pRoom = actor->getRoom();
        ImGui::Text("Room: %s", pRoom ? pRoom->getId().c_str() : "(none)");
        ImGui::Text("Talking: %s", actor->isTalking() ? "yes" : "no");
        ImGui::Text("Walking: %s", actor->isWalking() ? "yes" : "no");
        auto color = actor->getColor();
        if (ColorEdit4("Color", color))
        {
            actor->setColor(color);
        }
        auto talkColor = actor->getTalkColor();
        if (ColorEdit4("Talk color", talkColor))
        {
            actor->setTalkColor(talkColor);
        }
        auto offset = actor->getOffset();
        if (InputFloat2("Offset", offset))
        {
            actor->setOffset(offset);
        }
        auto walkSpeed = actor->getWalkSpeed();
        if (InputInt2("Walk speed", walkSpeed))
        {
            actor->setWalkSpeed(walkSpeed);
        }
        auto hotspotVisible = actor->isHotspotVisible();
        if (ImGui::Checkbox("Show hotspot", &hotspotVisible))
        {
            actor->showHotspot(hotspotVisible);
        }
        auto hotspot = actor->getHotspot();
        if (InputInt4("Hotspot", hotspot))
        {
            actor->setHotspot(hotspot);
        }
        ImGui::End();
    }

    void showObjects()
    {
        static auto objectGetter = [](void *vec, int idx, const char **out_text) {
            auto &vector = *static_cast<std::vector<std::unique_ptr<Object>> *>(vec);
            if (idx < 0 || idx >= static_cast<int>(vector.size()))
            {
                return false;
            }
            *out_text = tostring(vector.at(idx)->getId()).c_str();
            return true;
        };

        ImGui::Begin("Objects", &_showObjects);
        auto &objects = _engine.getRoom()->getObjects();
        ImGui::Combo("", &_selectedObject, objectGetter, static_cast<void *>(&objects), objects.size());
        if (!objects.empty())
        {
            auto &object = objects[_selectedObject];
            auto isVisible = object->isVisible();
            ImGui::TextUnformatted(tostring(object->getName()).c_str());
            if (ImGui::Checkbox("Visible", &isVisible))
            {
                object->setVisible(isVisible);
            }
            auto isTouchable = object->isTouchable();
            if (ImGui::Checkbox("Touchable", &isTouchable))
            {
                object->setTouchable(isTouchable);
            }
            auto zorder = object->getZOrder();
            if (ImGui::InputInt("Z-Order", &zorder))
            {
                object->setZOrder(zorder);
            }
            auto offset = object->getOffset();
            if (InputFloat2("Offset", offset))
            {
                object->setOffset(offset);
            }
            auto hotspotVisible = object->isHotspotVisible();
            if (ImGui::Checkbox("Show hotspot", &hotspotVisible))
            {
                object->showHotspot(hotspotVisible);
            }
            auto hotspot = object->getHotspot();
            if (InputInt4("Hotspot", hotspot))
            {
                object->setHotspot(hotspot);
            }
        }
        ImGui::End();
    }

    void showRooms()
    {
        ImGui::Begin("Rooms", &_showRooms);
        auto &rooms = _engine.getRooms();
        ImGui::Combo("", &_selectedRoom, roomGetter, static_cast<void *>(&rooms), rooms.size());
        auto &room = rooms[_selectedRoom];
        auto showWalkboxes = room->areDrawWalkboxesVisible();
        if (ImGui::Checkbox("Walkboxes", &showWalkboxes))
        {
            room->showDrawWalkboxes(showWalkboxes);
        }
        auto rotation = room->getRotation();
        if (ImGui::SliderFloat("rotation", &rotation, -180.f, 180.f, "%.0f deg"))
        {
            room->setRotation(rotation);
        }
        auto ambient = room->getAmbientLight();
        if (ColorEdit4("ambient", ambient))
        {
            room->setAmbientLight(ambient);
        }
        ImGui::End();
    }

    static bool roomGetter(void *vec, int idx, const char **out_text)
    {
        auto &vector = *static_cast<std::vector<std::unique_ptr<Room>> *>(vec);
        if (idx < 0 || idx >= static_cast<int>(vector.size()))
        {
            return false;
        }
        *out_text = vector.at(idx)->getId().c_str();
        return true;
    }

    static bool ColorEdit4(const char *label, sf::Color &color)
    {
        float imColor[4] = {color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
        if (ImGui::ColorEdit4(label, imColor))
        {
            color.r = static_cast<sf::Uint8>(imColor[0] * 255.f);
            color.g = static_cast<sf::Uint8>(imColor[1] * 255.f);
            color.b = static_cast<sf::Uint8>(imColor[2] * 255.f);
            color.a = static_cast<sf::Uint8>(imColor[3] * 255.f);
            return true;
        }
        return false;
    }

    static bool InputInt2(const char *label, sf::Vector2i &vector)
    {
        int vec[2] = {vector.x, vector.y};
        if (ImGui::InputInt2(label, vec))
        {
            vector.x = vec[0];
            vector.y = vec[1];
            return true;
        }
        return false;
    }

    static bool InputInt4(const char *label, sf::IntRect &rect)
    {
        int r[4] = {rect.left, rect.top, rect.width, rect.height};
        if (ImGui::InputInt4(label, r))
        {
            rect.left = r[0];
            rect.top = r[1];
            rect.width = r[2];
            rect.height = r[3];
            return true;
        }
        return false;
    }

    static bool InputFloat2(const char *label, sf::Vector2f &vector)
    {
        float vec[2] = {vector.x, vector.y};
        if (ImGui::InputFloat2(label, vec))
        {
            vector.x = vec[0];
            vector.y = vec[1];
            return true;
        }
        return false;
    }

  private:
    Engine &_engine;
    bool _showActors{true};
    bool _showObjects{true};
    bool _showRooms{false};
    int _selectedActor{0};
    int _selectedObject{0};
    int _selectedRoom{0};
    int _selectedStack{0};
};
} // namespace ng