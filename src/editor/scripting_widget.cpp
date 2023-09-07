#include "editor/scripting_widget.hpp"

#include <algorithm>

#include "editor/editor.hpp"
#include "interface/control_button.hpp"
#include "interface/label.hpp"
#include "supertux/game_object.hpp"
#include "supertux/globals.hpp"
#include "supertux/sector.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

static const float LABEL_HEIGHT = 20.f;
static const float BUTTON_SPACING = 10.f;
static const float SCROLLBAR_WIDTH = 10.f;

EditorScriptingWidget::EditorScriptingWidget(Editor& editor) :
  m_editor(editor),
  m_rect(),
  m_collapsed(true),
  m_mouse_pos(),
  m_scroll_progress(0.f),
  m_script_options(),
  m_controls(),
  m_labels(),
  m_scrollbar()
{
}

void
EditorScriptingWidget::setup()
{
  // Set initial values.
  m_rect = Rectf(Vector(), Sizef(static_cast<float>(SCREEN_WIDTH) / 2,
                                 static_cast<float>(SCREEN_HEIGHT) / 2));
  m_collapsed = true;
  m_scroll_progress = 0.f;
  m_script_options.clear();

  for (auto& obj : m_editor.get_sector()->get_objects())
    insert_object(obj.get());

  sort_objects();
  initialize_controls();
}

void
EditorScriptingWidget::insert_object(GameObject* obj)
{
  /** Determines all scripting object options of each object.
      Afterwards, adds the data from those options into the main map. */

  std::vector<ScriptOption> script_options;

  ObjectSettings settings = obj->get_settings();
  for (const auto& option : settings.get_options())
  {
    if (!option->is_script())
      continue;

    auto script_option = static_cast<ScriptObjectOption*>(option.get());
    script_options.push_back({ option->get_text(), script_option->get_pointer() });
  }

  if (!script_options.empty())
    m_script_options.push_back({ obj, std::move(script_options) });
}

void
EditorScriptingWidget::sort_objects()
{
  /** Singletons should be first, afterwards, all should be
      alphabetically sorted by class name and individual name. */
  std::sort(m_script_options.begin(), m_script_options.end(),
    [](const auto& lhs, const auto& rhs)
    {
      if (lhs.first->is_singleton() != rhs.first->is_singleton())
        return lhs.first->is_singleton();

      if (lhs.first->get_class_name() != rhs.first->get_class_name())
        return lhs.first->get_class_name() < rhs.first->get_class_name();

      return lhs.first->get_name() < rhs.first->get_name();
    });
}

void
EditorScriptingWidget::initialize_controls()
{
  // Clear any controls.
  m_controls.clear();
  m_labels.clear();

  /** Add all labels and controls. */
  float top = m_rect.get_top() + 20.f;
  for (const auto& script_option : m_script_options)
  {
    auto obj = script_option.first;

    // Add class and individual name label.
    auto label = std::make_unique<InterfaceLabel>();
    label->set_label(obj->get_class_name() + " \"" + obj->get_name() + "\"");
    label->set_rect(Rectf(Vector(20.f, top), Sizef(0.f, LABEL_HEIGHT)));
    const Rectf& label_rect = label->get_rect();
    m_labels.push_back(std::move(label));

    // Add buttons to edit script properties.
    float button_left = label_rect.get_right() + 20.f;
    const float button_width = ((m_rect.get_right() - label_rect.get_right()) -
                                  BUTTON_SPACING * static_cast<float>(script_option.second.size()) -
                                  SCROLLBAR_WIDTH - 20.f) /
                                static_cast<float>(script_option.second.size());
    for (auto& option : script_option.second)
    {
      auto button = std::make_unique<ControlButton>(option.name);
      button->set_rect(Rectf(Vector(button_left, top), Sizef(button_width, LABEL_HEIGHT)));
      button->m_on_change = [object = script_option.first, option]()
        {
          log_warning << object->get_name() << " " << option.name << " " << option.value << std::endl;
        };
      m_controls.push_back(std::move(button));

      button_left += button_width + BUTTON_SPACING;
    }

    top += LABEL_HEIGHT * 2;
  }

  /** Add scrollbar. */
  m_scrollbar.reset(new ControlScrollbar(top, m_rect.get_height(), m_scroll_progress));
  m_scrollbar->set_rect(Rectf(m_rect.get_right() - SCROLLBAR_WIDTH, m_rect.get_top(),
                              m_rect.get_right(), m_rect.get_bottom()));
}

void
EditorScriptingWidget::resize()
{
  m_rect.set_size(static_cast<float>(SCREEN_WIDTH) / 2,
                  static_cast<float>(SCREEN_HEIGHT) / 2);
}


void
EditorScriptingWidget::show()
{
  m_collapsed = false;
}

void
EditorScriptingWidget::hide()
{
  m_collapsed = true;
}


void
EditorScriptingWidget::draw(DrawingContext& context)
{
  context.color().draw_filled_rect(m_rect, Color::BLACK, LAYER_GUI - 1);

  context.push_transform();
  context.set_translation(Vector(0.f, m_scroll_progress));

  for (auto& control : m_controls)
  {
    if (control->get_rect().get_top() - m_scroll_progress > m_rect.get_bottom())
      continue;

    control->draw(context);
  }
  for (auto& label : m_labels)
  {
    if (label->get_rect().get_top() - m_scroll_progress > m_rect.get_bottom())
      continue;

    label->draw(context);
  }

  context.pop_transform();

  m_scrollbar->draw(context);
}

void
EditorScriptingWidget::update(float dt_sec)
{
  bool object_deleted = false;
  m_script_options.erase(std::remove_if(m_script_options.begin(), m_script_options.end(),
                                        [&object_deleted](const auto& script_option)
                                        {
                                          if (script_option.first->is_valid())
                                            return false;

                                          object_deleted = true;
                                          return true;
                                        }), m_script_options.end());

  if (object_deleted)
  {
    sort_objects();
    initialize_controls();
  }
}


bool
EditorScriptingWidget::on_mouse_button_up(const SDL_MouseButtonEvent& button)
{
  return m_scrollbar->on_mouse_button_up(button);
}

bool
EditorScriptingWidget::on_mouse_button_down(const SDL_MouseButtonEvent& button)
{
  return m_scrollbar->on_mouse_button_down(button);
}

bool
EditorScriptingWidget::on_mouse_motion(const SDL_MouseMotionEvent& motion)
{
  m_mouse_pos = VideoSystem::current()->get_viewport().to_logical(motion.x, motion.y);

  return m_scrollbar->on_mouse_motion(motion);
}

bool
EditorScriptingWidget::on_mouse_wheel(const SDL_MouseWheelEvent& wheel)
{
  if (!m_rect.contains(m_mouse_pos))
    return false;

  return m_scrollbar->on_mouse_wheel(wheel);
}


void
EditorScriptingWidget::add_object(GameObject* obj)
{
  insert_object(obj);
  sort_objects();
  initialize_controls();
}
