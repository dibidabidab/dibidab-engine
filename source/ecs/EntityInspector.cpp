
#include <gu/profiler.h>
#include <input/mouse_input.h>
#include <input/key_input.h>
#include <utils/code_editor/CodeEditor.h>
#include <imgui_internal.h>
#include "EntityInspector.h"
#include "../game/dibidab.h"
#include "entity_templates/LuaEntityTemplate.h"
#include "../generated/LuaScripted.hpp"
#include "../generated/Inspecting.hpp"


EntityInspector::EntityInspector(EntityEngine &engine, const std::string &name) : engine(engine), reg(engine.entities), inspectorName(name)
{

}

std::vector<std::string> inspectors;
std::string activeInspector;

static bool createEntityGUIJustOpened = false;

void EntityInspector::drawGUI(const Camera *cam, DebugLineRenderer &lineRenderer)
{
    gu::profiler::Zone z("entity inspector");

    inspectors.push_back(inspectorName);
    if (pickEntity)
    {
        pickEntityGUI(cam, lineRenderer);
        return;
    }
    if (moveEntity)
    {
        moveEntityGUI(cam, lineRenderer);
        return;
    }
    if (activeInspector == inspectorName)
    {
        pickEntity = KeyInput::justPressed(dibidab::settings.keyInput.inspectEntity);
        moveEntity = KeyInput::justPressed(dibidab::settings.keyInput.moveEntity);

        if (KeyInput::justPressed(dibidab::settings.keyInput.createEntity))
        {
            ImGui::OpenPopup("Create entity");
            createEntityGUIJustOpened = true;
        }

        if (ImGui::BeginPopup("Create entity"))
        {
            createEntityGUI();
            ImGui::EndPopup();
        }
    }
    reg.view<Inspecting>().each([&](auto e, Inspecting &ins) {
        drawEntityInspectorGUI(e, ins);
    });

    if (creatingTempl)
        templateArgsGUI();

    ImGui::BeginMainMenuBar();

    if (ImGui::BeginMenu(inspectorName.c_str()))
    {
        auto str = std::to_string(reg.alive()) + " entities active";
        ImGui::MenuItem(str.c_str(), "", false, false);

        if (ImGui::BeginMenu("Create entity"))
        {
            createEntityGUI();
            ImGui::EndMenu();
        }

        pickEntity |= ImGui::MenuItem("Inspect entity", KeyInput::getKeyName(dibidab::settings.keyInput.inspectEntity));
        moveEntity |= ImGui::MenuItem("Move entity", KeyInput::getKeyName(dibidab::settings.keyInput.moveEntity));

        if (ImGui::BeginMenu("Systems"))
        {
            for (auto sys : engine.getSystems())
                ImGui::MenuItem(sys->name.c_str(), NULL, &sys->enabled);

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

void EntityInspector::drawEntityInspectorGUI(entt::entity e, Inspecting &ins)
{
    if (!ins.show)
    {
        reg.remove<Inspecting>(e);
        return;
    }
    ImGui::SetNextWindowPos(ins.windowPos.x > 0 ? ins.windowPos : ImVec2(MouseInput::mouseX, MouseInput::mouseY), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(500, 680), ImGuiCond_Once);

    std::string title(inspectorName + " Entity #" + std::to_string(int(e)));
    if (!ImGui::Begin(title.c_str(), &ins.show, ImGuiWindowFlags_NoSavedSettings))
    {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }
    if (ImGui::Button("Destroy entity"))
    {
        reg.destroy(e);
        ImGui::End();
        return;
    }
    if (auto *luaScripted = reg.try_get<LuaScripted>(e))
    {
        if (luaScripted->usedTemplate != NULL)
        {
            ImGui::SameLine();
            if (ImGui::Button("Regenerate"))
            {
                vec3 pos = engine.getPosition(e);

                bool persistent = reg.try_get<Persistent>(e) != NULL;

                auto newE = luaScripted->usedTemplate->create(persistent);

                reg.get_or_assign<Inspecting>(newE).windowPos = ImGui::GetWindowPos();

                engine.setPosition(newE, pos);

                reg.destroy(e);
                ImGui::End();
                return;
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("This will:\n- Destroy this entity\n- Create a new entity using the '%s' template\n- Copy the position from this entity", luaScripted->usedTemplate->name.c_str());
        }
    }

    // ---- COMPONENTS TREE -------

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2,2));
    ImGui::Columns(2);
    ImGui::Separator();

    ImGui::Text("Components:");
    ImGui::NextColumn();
    std::string addComponentPopupName = "add_component_" + std::to_string(int(e));
    if (ImGui::Button("Add"))
        ImGui::OpenPopup(addComponentPopupName.c_str());
    drawAddComponent(e, ins, addComponentPopupName.c_str());

    ImGui::NextColumn();

    for (auto componentName : ComponentUtils::getAllComponentTypeNames())
    {
        auto componentUtils = ComponentUtils::getFor(componentName);
        if (!componentUtils->entityHasComponent(e, reg)) continue;

        ImGui::PushID(componentName.c_str());
        ImGui::AlignTextToFramePadding();

        bool dontRemove = true;
        bool component_open = ImGui::CollapsingHeader(componentName.c_str(), &dontRemove);
        if (!dontRemove)
        {
            componentUtils->removeComponent(e, reg);
            component_open = false;
        }
        ImGui::NextColumn();

        ImGui::NextColumn();
        if (component_open)
            drawComponentFieldsTree(e, ins, componentName.c_str(), componentUtils);

        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::PopStyleVar();

    ImGui::End();
}

void helpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool drawStringInput(std::string &str, Inspecting &ins, bool expandable=true, const char *label="")
{
    bool &multiline = ins.getState().multiline;

    ImGui::AlignTextToFramePadding();

    if (expandable)
    {
        if (!multiline && ImGui::Button(" v "))
            multiline = true;
        else if (multiline && ImGui::Button(" ^ "))
            multiline  = false;
    }
    ImGui::SameLine();

    const int extraBuffer = 1024;
    char *ptr = new char[str.length() + extraBuffer]();
    memcpy(ptr, &str[0], str.length());
    if (
            ((multiline && expandable) && ImGui::InputTextMultiline(label, ptr, str.length() + extraBuffer))
            ||
            (!(multiline && expandable) && ImGui::InputText(label, ptr, str.length() + extraBuffer))
            )
    {
        str = std::string(ptr);
        return ImGui::IsItemDeactivatedAfterEdit();
    }
    delete[] ptr;
    return false;
}

void drawJsonValue(json &value, Inspecting &ins, bool arrayPreview=true, bool readOnly=false)
{
    if (readOnly || (arrayPreview && value.is_array()))
    {
        std::string str = value.dump();
        ImGui::TextColored(ImVec4(.3, .5, 1, 1), "%s", str.c_str());
    }
    else if (value.is_number_float())
    {
        float v = value;
        if (ImGui::DragFloat(" ", &v))
            value = v;
        ImGui::SameLine();
        helpMarker("Drag or double-click");
    }
    else if (value.is_boolean())
    {
        bool v = value;
        if (ImGui::Checkbox(" ", &v))
            value = v;
    }
    else if (value.is_number_integer())
    {
        int v = value;
        if (ImGui::DragInt(" ", &v))
            value = v;
        ImGui::SameLine();
        helpMarker("Drag or double-click");
    }
    else if (value.is_string())
    {
        std::string v = value;
        drawStringInput(v, ins);
        value = v;
    }
}

void drawJsonTree(json &obj, Inspecting &ins, bool editStructure=true, bool readOnlyValues=false)
{
    int i = 0;
    int eraseI = -1;
    std::string eraseKey, addKey;
    json addJson;
    for (auto& [key, value] : obj.items()) { // also works for arrays: keys will be "0", "1", "2", etc.

        ins.currentPath.emplace_back(key);
        ImGui::PushID(i++);                      // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
        ImGui::AlignTextToFramePadding();  // Text and Tree nodes are less high than regular widgets, here we add vertical spacing to make the tree lines equal high.

        std::string keyStr = key, keyLabel = obj.is_array() ? "[" + key + "]" : (editStructure ? "" : key);

        bool field_open = value.is_structured();
        if (field_open)
            field_open = ImGui::TreeNode("Object", "%s", keyLabel.c_str());
        else
            ImGui::TreeNodeEx(
                    "Field", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_None,
                    "%s", keyLabel.c_str()
            );

        if (editStructure)
        {
            ImGui::SameLine();
            if (ImGui::Button(" x "))
            {
                eraseI = i;
                eraseKey = key;
            }
            if (obj.is_object())
            {
                ImGui::SameLine();
                if (drawStringInput(keyStr, ins, false, "     "))
                {
                    eraseI = i;
                    eraseKey = key;
                    addKey = keyStr;
                    addJson = value;
                }
            }
        }

        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();

        drawJsonValue(value, ins, true, readOnlyValues);

        ImGui::NextColumn();

        if (field_open)
        {
            drawJsonTree(value, ins, true, readOnlyValues);
            ImGui::TreePop();
        }
        ImGui::PopID();
        ins.currentPath.pop_back();
    }
    if (editStructure)
    {
        ImGui::Separator();
        ImGui::Text("Add: ");
        if (obj.is_array())
        {
            ImGui::NextColumn();
            if (ImGui::Button("float")) obj.push_back(float(0));
            ImGui::SameLine();
            if (ImGui::Button("int")) obj.push_back(int(0));
            ImGui::SameLine();
            if (ImGui::Button("string")) obj.push_back("");
            ImGui::SameLine();
            if (ImGui::Button("bool")) obj.push_back(bool(false));
            ImGui::SameLine();
            if (ImGui::Button("array")) obj.push_back(json::array());
            ImGui::SameLine();
            if (ImGui::Button("object")) obj.push_back(json::object());
            ImGui::SameLine();
            ImGui::NextColumn();
        } else
        {
            std::string &newKey = ins.getState().newKey;
            drawStringInput(newKey, ins, false, "  ");

            ImGui::NextColumn();
            if (ImGui::Button("float")) obj[newKey] = float(0);
            ImGui::SameLine();
            if (ImGui::Button("int")) obj[newKey] = int(0);
            ImGui::SameLine();
            if (ImGui::Button("string")) obj[newKey] = "";
            ImGui::SameLine();
            if (ImGui::Button("bool")) obj[newKey] = bool(false);
            ImGui::SameLine();
            if (ImGui::Button("array")) obj[newKey] = json::array();
            ImGui::SameLine();
            if (ImGui::Button("object")) obj[newKey] = json::object();
            ImGui::SameLine();
            ImGui::NextColumn();
        }
        ImGui::Separator();
    }
    if (eraseI > -1)
    {
        if (obj.is_array()) obj.erase(eraseI - 1);
        else obj.erase(eraseKey);
    }
    if (!addKey.empty())
        obj[addKey] = addJson;
}

void drawFieldsTree(
        json &valuesArray, const SerializableStructInfo *info, Inspecting &ins,
        bool readOnly=false, bool forceEditReadOnly=false
)
{
    for (int i = 0; i < info->nrOfFields; i++)
    {
        auto fieldName = info->fieldNames[i];
        auto fieldTypeName = info->fieldTypeNames[i];

        ImGui::PushID(fieldName);                      // Use object uid as identifier. Most commonly you could also use the object pointer as a base ID.
        ImGui::AlignTextToFramePadding();  // Text and Tree nodes are less high than regular widgets, here we add vertical spacing to make the tree lines equal high.

        json &value = valuesArray[i];

        bool field_open = value.is_structured();
        if (field_open)
            field_open = ImGui::TreeNode("Object", "%s", fieldName);
        else
            ImGui::TreeNodeEx(
                    "Field", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_None,
                    "%s", fieldName
            );

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(.3, .5, .7, 1), "%s", fieldTypeName);

        ImGui::NextColumn();
        ImGui::AlignTextToFramePadding();

        auto subInfo = SerializableStructInfo::getFor(fieldTypeName);

        static std::string finalTypeBegin = "final<";
        bool subReadOnly = !forceEditReadOnly
                            && (readOnly
                                || std::string(fieldTypeName).compare(0, finalTypeBegin.length(), finalTypeBegin) == 0
                            );

        drawJsonValue(value, ins, !subInfo, subReadOnly);

        ImGui::NextColumn();
        if (field_open)
        {
            ins.currentPath.emplace_back(fieldName);
            if (subInfo)
                drawFieldsTree(value, subInfo, ins, subReadOnly);
            else drawJsonTree(
                    value, ins,
                    !info->structFieldIsFixedSize[i] && !subReadOnly,
                    subReadOnly
            );
            ins.currentPath.pop_back();
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
}

void EntityInspector::drawComponentFieldsTree(entt::entity e, Inspecting &ins, const char *componentName, const ComponentUtils *componentUtils)
{
    auto info = SerializableStructInfo::getFor(componentName);
    json valuesArray;
    componentUtils->getJsonComponent(valuesArray, e, reg);

    assert(ins.currentPath.empty());
    ins.currentPath.emplace_back(componentName);
    drawFieldsTree(valuesArray, info, ins);
    ins.currentPath.pop_back();

    try {
        componentUtils->setJsonComponent(valuesArray, e, reg);
    } catch (std::exception& e) {
        std::cerr << "Exception after editing component in inspector:\n" << e.what() << std::endl;
    }
}

void EntityInspector::drawAddComponent(entt::entity e, Inspecting &ins, const char *popupName)
{
    if (ImGui::BeginPopup(popupName))
    {
        ImGui::Text("Component Type:");
        ImGui::Separator();

        for (auto typeName : ComponentUtils::getAllComponentTypeNames())
        {
            auto utils = ComponentUtils::getFor(typeName);
            if (utils->entityHasComponent(e, reg))
                continue;
            if (ImGui::Selectable(typeName.c_str()))
            {
                ins.addingComponentTypeName = typeName;
                ins.addingComponentJson = utils->getDefaultJsonComponent();
            }
        }
        ImGui::EndPopup();
    }

    if (ins.addingComponentTypeName.empty()) return;

    ImGui::SetNextWindowPos(ImVec2(MouseInput::mouseX - 200, MouseInput::mouseY - 15), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    std::string title = "Adding " + ins.addingComponentTypeName + " to entity #" + std::to_string(int(e));
    bool open = true;
    ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoSavedSettings);

    ImGui::Text("[Warning]:\nadding components (with wrong values) could crash the game!");
    ImGui::Columns(2);
    ImGui::Separator();

    drawFieldsTree(ins.addingComponentJson, SerializableStructInfo::getFor(ins.addingComponentTypeName.c_str()), ins, false);

    ImGui::Columns(1);
    ImGui::Separator();

    if (ImGui::Button("Add"))
    {
        open = false;

        auto utils = ComponentUtils::getFor(ins.addingComponentTypeName);
        try
        {
            utils->setJsonComponent(ins.addingComponentJson, e, reg);
        } catch (std::exception& e) {
            std::cerr << "Exception while trying to add component in inspector:\n" << e.what() << std::endl;
        }
    }
    ImGui::End();
    if (!open)
        ins.addingComponentTypeName.clear();
}

bool createPersistent = false;

#define HOVERED_AND_PRESSED_ENTER (ImGui::IsItemHovered() && ImGui::IsKeyPressed(GLFW_KEY_ENTER))

void EntityInspector::createEntityGUI()
{
    static ImGuiTextFilter filter;
    if (ImGui::IsKeyPressed(GLFW_KEY_ESCAPE))
    {
        bool close = !filter.IsActive();
        filter = ImGuiTextFilter();
        if (close)
        {
            ImGui::CloseCurrentPopup();
            return;
        }
    }

    if (ImGui::MenuItem("Empty entity"))
        reg.assign<Inspecting>(reg.create());

    ImGui::Separator();
    ImGui::MenuItem("From template:", NULL, false, false);

    static bool _ = false;
    if (_)
    {
        ImGui::SetKeyboardFocusHere();
        _ = false;
    }
    if (createEntityGUIJustOpened)
    {
        _ = true;
        createEntityGUIJustOpened = false;
    }
    filter.Draw("Filter", 200);
    ImGui::NewLine();

    for (auto &templateName : engine.getTemplateNames())
    {
        if (!filter.PassFilter(templateName.c_str()))
            continue;

        auto name = templateName;
        const char *description = NULL;

        auto templ = &engine.getTemplate(templateName);
        auto luaTempl = dynamic_cast<LuaEntityTemplate *>(templ);
        if (luaTempl)
        {
            name = splitString(luaTempl->script.getLoadedAsset().shortPath, "scripts/entities/").back();
            if (!luaTempl->getDescription().empty())
                description = luaTempl->getDescription().c_str();
        }

        bool show = true;

        auto dirSplitted = splitString(name, "/");

        int subMenusOpened = 0;
        if (filter.Filters.empty()) for (int i = 0; i < dirSplitted.size() - 1; i++)
        {
            if (i == 0 && dirSplitted[0] == createEntity_showSubFolder)
                continue;

            ImGui::SetNextWindowContentSize(ImVec2(240, 0));
            if (!ImGui::BeginMenu(dirSplitted[i].c_str()))
            {
                show = false;
                break;
            }
            else subMenusOpened++;
        }

        if (show)
        {
            ImGui::Columns(2, NULL, false);
            ImGui::SetColumnWidth(0, 120);
            ImGui::SetColumnWidth(1, 120);

            bool create = false;

            if (ImGui::MenuItem(dirSplitted.back().c_str(), NULL) || HOVERED_AND_PRESSED_ENTER)
            {
                create = true;
                createPersistent = false;
            }

            if (description && ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", description);

            ImGui::NextColumn();
            if (!createEntity_persistentOption)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }
            if (ImGui::Button(("Persistent###pers_" + name).c_str()) || HOVERED_AND_PRESSED_ENTER)
            {
                create = true;
                createPersistent = true;
            }
            if (!createEntity_persistentOption)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Save the created entity to level file");

            if (luaTempl)
            {
                ImGui::SameLine();
                auto str = "Edit###edit_" + name;
                if (ImGui::Button(str.c_str()) || HOVERED_AND_PRESSED_ENTER)
                    editLuaScript(luaTempl);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Edit the Lua script for this entity");
            }

            ImGui::Columns(1);

            if (create)
                createEntity(dirSplitted.back());
        }

        for (int i = 0; i < subMenusOpened; i++)
            ImGui::EndMenu();
    }
}

void EntityInspector::createEntity(const std::string &templateName)
{
    auto templ = &engine.getTemplate(templateName);

    auto *luaTempl = dynamic_cast<LuaEntityTemplate *>(templ);

    if (luaTempl && !luaTempl->getDefaultArgs().empty())
    {
        creatingTempl = luaTempl;
        creatingTemplArgs = luaTempl->getDefaultArgs();
    }
    else
    {
        moveEntity = true;
        movingEntity = templ->create(createPersistent);
    }
}

void EntityInspector::templateArgsGUI()
{
    ImGui::SetNextWindowPos(ImVec2(MouseInput::mouseX - 200, MouseInput::mouseY - 15), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Once);

    const std::string &name = creatingTempl->script.getLoadedAsset().shortPath;
    std::string title = "Creating " + name;
    bool open = !ImGui::IsKeyPressed(GLFW_KEY_ESCAPE);
    ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoSavedSettings);

    ImGui::Columns(2);
    ImGui::Separator();

    static Inspecting ins;
    drawJsonTree(creatingTemplArgs, ins, false);

    ImGui::Columns(1);
    ImGui::Separator();

    if (createEntity_persistentOption)
    {
        ImGui::Checkbox("Persistent", &createPersistent);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Save the created entity to level file");
    }

    if (ImGui::Button("Create") || HOVERED_AND_PRESSED_ENTER)
    {
        open = false;
        auto e = reg.create();
        creatingTempl->createComponentsWithJsonArguments(e, creatingTemplArgs, createPersistent);
        moveEntity = true;
        movingEntity = e;
    }
    ImGui::SetItemDefaultFocus();

    ImGui::End();
    if (!open)
    {
        creatingTempl = NULL;
        creatingTemplArgs.clear();
    }
}

void EntityInspector::editLuaScript(LuaEntityTemplate *luaTemplate)
{
    auto script = luaTemplate->script;
    for (auto &t : CodeEditor::tabs)
        if (t.title == script.getLoadedAsset().fullPath)
            return;

    auto &tab = CodeEditor::tabs.emplace_back();
    tab.title = script.getLoadedAsset().fullPath;
    tab.code = File::readString(script.getLoadedAsset().fullPath.c_str());
    tab.languageDefinition = TextEditor::LanguageDefinition::C(); // the lua definition is pretty broken
    tab.save = [script] (auto &tab) {

        File::writeBinary(script.getLoadedAsset().fullPath.c_str(), tab.code); // todo: why is this called writeBINARY? lol

        AssetManager::loadFile(script.getLoadedAsset().fullPath, "assets/");
    };
    tab.revert = [script] (auto &tab) {
        tab.code = File::readString(script.getLoadedAsset().fullPath.c_str());
    };
}

void EntityInspector::drawInspectingDropDown()
{
    if (inspectors.empty())
        return;

    ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_Button), "    Inspecting:");
    ImGui::SetNextItemWidth(120);

    auto prevInspector = activeInspector;
    activeInspector.clear();

    bool draw = ImGui::BeginCombo("    ", prevInspector.c_str());
    for (auto &name : inspectors)
    {
        if (activeInspector.empty() || prevInspector == name)
            activeInspector = name;

        if (draw && ImGui::Selectable(name.c_str()))
        {
            activeInspector = name;
            break;
        }
    }

    if (draw)
        ImGui::EndCombo();

    inspectors.clear();
}
