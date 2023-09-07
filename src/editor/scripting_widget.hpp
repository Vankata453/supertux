#include "editor/widget.hpp"

#include <memory>
#include <vector>

#include "interface/control_scrollbar.hpp"
#include "math/rectf.hpp"

class Editor;
class GameObject;
class InterfaceControl;
class InterfaceLabel;

class EditorScriptingWidget final : public Widget
{
public:
  EditorScriptingWidget(Editor& editor);

  void setup() override;
  void resize() override;

  void show();
  void hide();

  void draw(DrawingContext& context) override;
  void update(float dt_sec) override;

  bool on_mouse_button_up(const SDL_MouseButtonEvent& button) override;
  bool on_mouse_button_down(const SDL_MouseButtonEvent& button) override;
  bool on_mouse_motion(const SDL_MouseMotionEvent& motion) override;
  bool on_mouse_wheel(const SDL_MouseWheelEvent& wheel) override;

  void add_object(GameObject* obj);

private:
  void insert_object(GameObject* obj);
  void sort_objects();
  void initialize_controls();

private:
  struct ScriptOption
  {
    const std::string name;
    const std::string* value;
  };

private:
  Editor& m_editor;

  Rectf m_rect;
  bool m_collapsed;
  Vector m_mouse_pos;
  float m_scroll_progress;

  std::vector<std::pair<GameObject*, std::vector<ScriptOption>>> m_script_options;

  std::vector<std::unique_ptr<InterfaceControl>> m_controls;
  std::vector<std::unique_ptr<InterfaceLabel>> m_labels;
  std::unique_ptr<ControlScrollbar> m_scrollbar;

private:
  EditorScriptingWidget(const EditorScriptingWidget&) = delete;
  EditorScriptingWidget& operator=(const EditorScriptingWidget&) = delete;
};
