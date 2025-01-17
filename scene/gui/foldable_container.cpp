/**************************************************************************/
/*  foldable_container.cpp                                                */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "foldable_container.h"

#include "scene/theme/theme_db.h"

Size2 FoldableContainer::get_minimum_size() const {
	Size2 title_ms = _get_title_panel_min_size();

	if (!expanded) {
		return title_ms;
	}
	Size2 ms;

	for (int i = 0; i < get_child_count(); i++) {
		Control *c = Object::cast_to<Control>(get_child(i));
		if (!c || !c->is_visible()) {
			continue;
		}
		if (c->is_set_as_top_level()) {
			continue;
		}
		ms = ms.max(c->get_combined_minimum_size());
	}
	ms += theme_cache.panel_style->get_minimum_size();

	return Size2(MAX(ms.width, title_ms.width), ms.height + title_ms.height);
}

Vector<int> FoldableContainer::get_allowed_size_flags_horizontal() const {
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

Vector<int> FoldableContainer::get_allowed_size_flags_vertical() const {
	Vector<int> flags;
	flags.append(SIZE_FILL);
	flags.append(SIZE_SHRINK_BEGIN);
	flags.append(SIZE_SHRINK_CENTER);
	flags.append(SIZE_SHRINK_END);
	return flags;
}

void FoldableContainer::set_expanded(bool p_expanded) {
	if (expanded == p_expanded) {
		return;
	}
	expanded = p_expanded;

	for (int i = 0; i < get_child_count(); i++) {
		Control *c = Object::cast_to<Control>(get_child(i));
		if (!c || c->is_set_as_top_level()) {
			continue;
		}
		c->set_visible(expanded);
	}
	update_minimum_size();
	queue_redraw();
	emit_signal(SNAME("folding_changed"), !expanded);
}

bool FoldableContainer::is_expanded() const {
	return expanded;
}

void FoldableContainer::set_title(const String &p_title) {
	if (title == p_title) {
		return;
	}
	title = p_title;
	_shape();
	update_minimum_size();
	queue_redraw();
}

String FoldableContainer::get_title() const {
	return title;
}

void FoldableContainer::set_title_alignment(HorizontalAlignment p_alignment) {
	ERR_FAIL_INDEX((int)p_alignment, 3);

	title_alignment = p_alignment;

	if (_get_actual_alignment() != text_buf->get_horizontal_alignment()) {
		_shape();
		queue_redraw();
	}
}

HorizontalAlignment FoldableContainer::get_title_alignment() const {
	return title_alignment;
}

void FoldableContainer::set_language(const String &p_language) {
	if (language == p_language) {
		return;
	}
	language = p_language;
	_shape();
	update_minimum_size();
	queue_redraw();
}

String FoldableContainer::get_language() const {
	return language;
}

void FoldableContainer::set_text_direction(Control::TextDirection p_text_direction) {
	ERR_FAIL_COND((int)p_text_direction < -1 || (int)p_text_direction > 3);
	if (text_direction == p_text_direction) {
		return;
	}
	text_direction = p_text_direction;

	_shape();
	queue_redraw();
}

Control::TextDirection FoldableContainer::get_text_direction() const {
	return text_direction;
}

void FoldableContainer::set_text_overrun_behavior(TextServer::OverrunBehavior p_overrun_behavior) {
	if (overrun_behavior == p_overrun_behavior) {
		return;
	}
	overrun_behavior = p_overrun_behavior;

	_shape();
	update_minimum_size();
	queue_redraw();
}

TextServer::OverrunBehavior FoldableContainer::get_text_overrun_behavior() const {
	return overrun_behavior;
}

void FoldableContainer::set_title_position(FoldableContainer::TitlePosition p_title_position) {
	ERR_FAIL_INDEX(p_title_position, POSITION_MAX);
	if (title_position != p_title_position) {
		title_position = p_title_position;
		queue_redraw();
		queue_sort();
	}
}

FoldableContainer::TitlePosition FoldableContainer::get_title_position() const {
	return title_position;
}

void FoldableContainer::add_button(const Ref<Texture2D> &p_icon, int p_position, int p_id) {
	Button button = Button();
	button.icon = p_icon;
	button.id = p_id == -1 ? buttons.size() : p_id;
	p_position = p_position < 0 ? MAX(buttons.size(), 0) : CLAMP(p_position, 0, buttons.size());

	buttons.insert(p_position, button);
	update_minimum_size();
	queue_redraw();
}

void FoldableContainer::remove_button(int p_index) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	bool is_visible = !buttons[p_index].hidden;
	buttons.remove_at(p_index);

	if (is_visible) {
		update_minimum_size();
		queue_redraw();
	}
}

int FoldableContainer::get_button_count() const {
	return buttons.size();
}

Rect2 FoldableContainer::get_button_rect(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), Rect2());

	return buttons[p_index].rect;
}

void FoldableContainer::set_button_id(int p_index, int p_id) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	buttons.write[p_index].id = p_id;
}

int FoldableContainer::get_button_id(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), -1);

	return buttons[p_index].id;
}

int FoldableContainer::move_button(int p_from, int p_to) {
	int arr_size = buttons.size();
	ERR_FAIL_INDEX_V(p_from, arr_size, -1);
	ERR_FAIL_COND_V(arr_size < 2, -1);
	p_to = p_to == -1 ? arr_size - 1 : CLAMP(p_to, 0, arr_size - 1);
	ERR_FAIL_COND_V(p_from == p_to, -1);

	Button button = buttons[p_from];
	buttons.remove_at(p_from);
	arr_size--;
	p_to = CLAMP(p_to, 0, arr_size);

	return buttons.insert(p_to, button) == OK ? p_to : -1;
}

int FoldableContainer::get_button_index(int p_id) const {
	for (int i = 0; i < buttons.size(); i++) {
		if (buttons[i].id == p_id) {
			return i;
		}
	}
	return -1;
}

void FoldableContainer::set_button_toggle_mode(int p_index, bool p_mode) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	buttons.write[p_index].toggle_mode = p_mode;
	if (!p_mode && buttons[p_index].toggled_on) {
		buttons.write[p_index].toggled_on = false;
		queue_redraw();
	}
}

int FoldableContainer::get_button_toggle_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), false);

	return buttons[p_index].toggle_mode;
}

void FoldableContainer::set_button_toggled(int p_index, bool p_toggled_on) {
	ERR_FAIL_INDEX(p_index, buttons.size());
	ERR_FAIL_COND(!buttons[p_index].toggle_mode);

	if (buttons[p_index].toggled_on != p_toggled_on) {
		buttons.write[p_index].toggled_on = p_toggled_on;
		queue_redraw();
	}
}

bool FoldableContainer::is_button_toggled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), false);

	return buttons[p_index].toggled_on;
}

void FoldableContainer::set_button_icon(int p_index, const Ref<Texture2D> &p_icon) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	buttons.write[p_index].icon = p_icon;

	if (!buttons[p_index].hidden) {
		update_minimum_size();
		queue_redraw();
	}
}

Ref<Texture2D> FoldableContainer::get_button_icon(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), Ref<Texture2D>());

	return buttons[p_index].icon;
}

void FoldableContainer::set_button_tooltip(int p_index, String p_tooltip) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	buttons.write[p_index].tooltip = p_tooltip;
}

String FoldableContainer::get_button_tooltip(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), "");

	return buttons[p_index].tooltip;
}

void FoldableContainer::set_button_disabled(int p_index, bool p_disabled) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	if (buttons[p_index].disabled == p_disabled) {
		return;
	}

	buttons.write[p_index].disabled = p_disabled;

	if (!buttons[p_index].hidden) {
		update_minimum_size();
		queue_redraw();
	}
}

bool FoldableContainer::is_button_disabled(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), false);

	return buttons[p_index].disabled;
}

void FoldableContainer::set_button_hidden(int p_index, bool p_hidden) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	if (buttons[p_index].hidden == p_hidden) {
		return;
	}

	buttons.write[p_index].hidden = p_hidden;

	update_minimum_size();
	queue_redraw();
}

bool FoldableContainer::is_button_hidden(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), false);

	return buttons[p_index].hidden;
}

void FoldableContainer::set_button_metadata(int p_index, Variant p_metadata) {
	ERR_FAIL_INDEX(p_index, buttons.size());

	buttons.write[p_index].metadata = p_metadata;
}

Variant FoldableContainer::get_button_metadata(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, buttons.size(), Variant());

	return buttons[p_index].metadata;
}

int FoldableContainer::get_button_at_position(const Point2 &p_pos) const {
	for (int i = 0; i < buttons.size(); i++) {
		if (buttons[i].disabled || buttons[i].hidden) {
			continue;
		}
		if (buttons[i].rect.has_point(p_pos)) {
			return i;
		}
	}
	return -1;
}

void FoldableContainer::gui_input(const Ref<InputEvent> &p_event) {
	ERR_FAIL_COND(p_event.is_null());

	Ref<InputEventMouseMotion> m = p_event;

	if (m.is_valid()) {
		int _last_hovered = _hovered;
		_hovered = -1;
		Rect2 title_rect = Rect2(0, (title_position == POSITION_TOP) ? 0 : get_size().height - title_panel_height, get_size().width, title_panel_height);
		if (title_rect.has_point(m->get_position())) {
			if (!is_hovering) {
				is_hovering = true;
				queue_redraw();
			}
			_hovered = get_button_at_position(m->get_position());
		} else if (is_hovering) {
			is_hovering = false;
			queue_redraw();
		}
		if (_last_hovered != _hovered) {
			queue_redraw();
		}
	}

	if (has_focus() && p_event->is_action_pressed("ui_accept", false, true)) {
		set_expanded(!expanded);
		accept_event();
		return;
	}

	Ref<InputEventMouseButton> b = p_event;
	if (b.is_valid()) {
		int _last_pressed = _pressed;
		_pressed = -1;
		Rect2 title_rect = Rect2(0, (title_position == POSITION_TOP) ? 0 : get_size().height - title_panel_height, get_size().width, title_panel_height);
		if (b->get_button_index() == MouseButton::LEFT && title_rect.has_point(b->get_position())) {
			int button_pos = get_button_at_position(b->get_position());
			if (b->is_pressed()) {
				_pressed = button_pos;
				if (_pressed == -1) {
					set_expanded(!expanded);
				}
				accept_event();
			} else {
				if (button_pos > -1 && button_pos == _last_pressed) {
					if (buttons[_last_pressed].toggle_mode) {
						bool toggled_on = buttons[_last_pressed].toggled_on;
						buttons.write[_last_pressed].toggled_on = !toggled_on;
						emit_signal(SNAME("button_toggled"), !toggled_on, get_button_id(_last_pressed), _last_pressed);
					} else {
						emit_signal(SNAME("button_pressed"), get_button_id(_last_pressed), _last_pressed);
					}
				}
				accept_event();
			}
		}
		if (_last_pressed != _pressed) {
			queue_redraw();
		}
	}
}

String FoldableContainer::get_tooltip(const Point2 &p_pos) const {
	if (p_pos.y <= title_panel_height) {
		if (_hovered != -1) {
			return buttons[_hovered].tooltip;
		}
		return Control::get_tooltip(p_pos);
	}
	return "";
}

void FoldableContainer::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_DRAW: {
			RID ci = get_canvas_item();
			Size2 size = get_size();
			title_panel_height = _get_title_panel_min_size().height;
			Ref<StyleBox> title_style = _get_title_style();

			Point2 title_ofs = (title_position == POSITION_TOP) ? Point2() : Point2(0, size.height - title_panel_height);
			Rect2 title_rect = Rect2(-title_ofs, Size2(size.width, title_panel_height));
			if (title_position == POSITION_BOTTOM) {
				draw_set_transform(Point2(0.0, title_style->get_draw_rect(title_rect).size.height), 0.0, Size2(1.0, -1.0));
			}
			title_style->draw(ci, title_rect);
			if (title_position == POSITION_BOTTOM) {
				draw_set_transform(Point2(), 0.0, Size2(1.0, 1.0));
			}

			Size2 title_ms = title_style->get_minimum_size();
			int title_width = size.width - title_ms.width;

			int h_separation = MAX(theme_cache.h_separation, 0);
			int title_style_ofs = (title_position == POSITION_TOP) ? title_style->get_margin(SIDE_TOP) : title_style->get_margin(SIDE_BOTTOM);
			Point2 title_pos;
			title_pos.y = MAX((title_panel_height - title_ms.height - text_buf->get_size().height) * 0.5, 0) + title_style_ofs;

			Ref<Texture2D> icon = _get_title_icon();

			title_width -= icon->get_width() + h_separation;
			Point2 icon_pos;
			icon_pos.x = title_style->get_margin(SIDE_LEFT);
			icon_pos.y = MAX((title_panel_height - title_ms.height - icon->get_height()) * 0.5, 0) + title_style_ofs;

			bool rtl = is_layout_rtl();
			if (rtl) {
				icon_pos.x = size.width - icon_pos.x - icon->get_width();
			} else {
				title_pos.x = title_style->get_margin(SIDE_LEFT) + icon->get_width() + h_separation;
			}
			icon->draw(ci, title_ofs + icon_pos);

			Size2 button_ms = theme_cache.button_normal_style->get_minimum_size();
			int offset = 0;
			for (int i = buttons.size() - 1; i > -1; i--) {
				Button button = buttons[i];
				if (button.hidden || button.icon.is_null()) {
					continue;
				}

				Ref<StyleBox> button_style;
				Color icon_color;
				if (button.disabled) {
					button_style = theme_cache.button_disabled_style;
					icon_color = theme_cache.button_icon_normal;
				} else if (i == _pressed || button.toggled_on) {
					button_style = theme_cache.button_pressed_style;
					icon_color = theme_cache.button_icon_pressed;
				} else if (i == _hovered) {
					button_style = theme_cache.button_hovered_style;
					icon_color = button.toggled_on ? theme_cache.button_icon_pressed : theme_cache.button_icon_hovered;
				} else {
					button_style = theme_cache.button_normal_style;
					icon_color = theme_cache.button_icon_normal;
				}

				Size2 icon_size = button.icon->get_size();
				Size2 button_size = icon_size + button_ms;
				Point2 button_pos;
				button_pos.x = rtl ? title_style->get_margin(SIDE_LEFT) + offset : size.width - button_size.width - title_style->get_margin(SIDE_RIGHT) - offset;
				button_pos.y = MAX((title_panel_height - title_ms.height - button_size.height) * 0.5, 0) + title_style_ofs;
				button_style->draw(ci, Rect2(title_ofs + button_pos, button_size));
				button.icon->draw(ci, title_ofs + button_pos + button_style->get_offset(), icon_color);
				buttons.write[i].rect = Rect2(title_ofs + button_pos, button_size);
				offset += icon_size.width + button_ms.width + h_separation;
			}

			title_width -= offset;
			if (rtl) {
				title_pos.x = offset + h_separation;
			}

			Color font_color = expanded ? theme_cache.title_font_color : theme_cache.title_collapsed_font_color;
			if (is_hovering) {
				font_color = theme_cache.title_hovered_font_color;
			}
			text_buf->set_width(title_width);

			if (title_width > 0) {
				if (theme_cache.title_font_outline_size > 0 && theme_cache.title_font_outline_color.a > 0) {
					text_buf->draw_outline(ci, title_ofs + title_pos, theme_cache.title_font_outline_size, theme_cache.title_font_outline_color);
				}
				text_buf->draw(ci, title_ofs + title_pos, font_color);
			}

			if (expanded) {
				Rect2 panel_rect = Rect2(Point2(0, (title_position == POSITION_TOP) ? title_panel_height : 0), Size2(size.width, size.height - title_panel_height));
				if (title_position == POSITION_BOTTOM) {
					draw_set_transform(Point2(0.0, title_style->get_draw_rect(panel_rect).size.height), 0.0, Size2(1.0, -1.0));
				}
				theme_cache.panel_style->draw(ci, panel_rect);
				if (title_position == POSITION_BOTTOM) {
					draw_set_transform(Point2(), 0.0, Size2(1.0, 1.0));
				}
			}

			if (has_focus()) {
				Rect2 focus_rect = expanded ? Rect2(Point2(), size) : Rect2(-title_ofs, Size2(size.width, title_panel_height));
				if (title_position == POSITION_BOTTOM) {
					draw_set_transform(Point2(0.0, title_style->get_draw_rect(focus_rect).size.height), 0.0, Size2(1.0, -1.0));
				}
				theme_cache.focus_style->draw(ci, focus_rect);
				if (title_position == POSITION_BOTTOM) {
					draw_set_transform(Point2(), 0.0, Size2(1.0, 1.0));
				}
			}
		} break;

		case NOTIFICATION_SORT_CHILDREN: {
			Size2 size = get_size() - theme_cache.panel_style->get_minimum_size();
			size.height -= title_panel_height;

			Point2 ofs;
			ofs.x = is_layout_rtl() ? theme_cache.panel_style->get_margin(SIDE_RIGHT) : theme_cache.panel_style->get_margin(SIDE_LEFT);
			if (title_position == POSITION_TOP) {
				ofs.y = title_panel_height + theme_cache.panel_style->get_margin(SIDE_TOP);
			} else {
				ofs.y = theme_cache.panel_style->get_margin(SIDE_BOTTOM);
			}

			for (int i = 0; i < get_child_count(); i++) {
				Control *c = Object::cast_to<Control>(get_child(i));
				if (!c || !c->is_visible_in_tree()) {
					continue;
				}
				if (c->is_set_as_top_level()) {
					continue;
				}
				c->set_visible(expanded);
				fit_child_in_rect(c, Rect2(ofs, size));
			}
		} break;

		case NOTIFICATION_MOUSE_EXIT: {
			if (is_hovering || _hovered != -1) {
				is_hovering = false;
				_hovered = -1;
				queue_redraw();
			}
		} break;

		case NOTIFICATION_LAYOUT_DIRECTION_CHANGED:
		case NOTIFICATION_TRANSLATION_CHANGED:
		case NOTIFICATION_THEME_CHANGED: {
			_shape();
			update_minimum_size();
			queue_redraw();
		} break;
	}
}

Size2 FoldableContainer::_get_title_panel_min_size() const {
	Ref<StyleBox> title_style = expanded ? theme_cache.title_style : theme_cache.title_collapsed_style;
	Ref<Texture2D> icon = _get_title_icon();
	Size2 title_ms = title_style->get_minimum_size();
	Size2 s = title_ms;
	s.width += icon->get_width();

	if (title.length() > 0) {
		s.width += MAX(0, theme_cache.h_separation);
		Size2 text_size = text_buf->get_size();
		s.height += text_size.height;
		if (overrun_behavior == TextServer::OverrunBehavior::OVERRUN_NO_TRIMMING) {
			s.width += text_size.width;
		}
	}

	Size2 button_ms = theme_cache.button_normal_style->get_minimum_size();
	int icon_height = 0;
	for (Button button : buttons) {
		if (button.hidden) {
			continue;
		}
		s.width += theme_cache.h_separation + button_ms.width;
		Size2 icon_size = button.icon->get_size();
		s.width += icon_size.width;
		icon_height = MAX(icon_height, icon_size.height);
	}

	if (icon_height > 0) {
		s.height = MAX(s.height, title_ms.height + button_ms.height + icon_height);
	}
	s.height = MAX(s.height, title_ms.height + icon->get_height());

	return s;
}

Ref<StyleBox> FoldableContainer::_get_title_style() const {
	if (is_hovering) {
		return expanded ? theme_cache.title_hover_style : theme_cache.title_collapsed_hover_style;
	}
	return expanded ? theme_cache.title_style : theme_cache.title_collapsed_style;
}

Ref<Texture2D> FoldableContainer::_get_title_icon() const {
	if (expanded) {
		return (title_position == POSITION_TOP) ? theme_cache.arrow : theme_cache.arrow_mirrored;
	} else if (is_layout_rtl()) {
		return theme_cache.arrow_collapsed_mirrored;
	}
	return theme_cache.arrow_collapsed;
}

void FoldableContainer::_shape() {
	text_buf->clear();
	text_buf->set_width(-1);

	if (text_direction == Control::TEXT_DIRECTION_INHERITED) {
		text_buf->set_direction(is_layout_rtl() ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR);
	} else {
		text_buf->set_direction((TextServer::Direction)text_direction);
	}

	text_buf->set_horizontal_alignment(_get_actual_alignment());

	text_buf->set_text_overrun_behavior(overrun_behavior);

	text_buf->add_string(atr(title), theme_cache.title_font, theme_cache.title_font_size, language);
}

HorizontalAlignment FoldableContainer::_get_actual_alignment() const {
	if (is_layout_rtl()) {
		if (title_alignment == HORIZONTAL_ALIGNMENT_RIGHT) {
			return HORIZONTAL_ALIGNMENT_LEFT;
		} else if (title_alignment == HORIZONTAL_ALIGNMENT_LEFT) {
			return HORIZONTAL_ALIGNMENT_RIGHT;
		}
	}
	return title_alignment;
}

void FoldableContainer::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_expanded", "expanded"), &FoldableContainer::set_expanded);
	ClassDB::bind_method(D_METHOD("is_expanded"), &FoldableContainer::is_expanded);
	ClassDB::bind_method(D_METHOD("set_title", "title"), &FoldableContainer::set_title);
	ClassDB::bind_method(D_METHOD("get_title"), &FoldableContainer::get_title);
	ClassDB::bind_method(D_METHOD("set_title_alignment", "alignment"), &FoldableContainer::set_title_alignment);
	ClassDB::bind_method(D_METHOD("get_title_alignment"), &FoldableContainer::get_title_alignment);
	ClassDB::bind_method(D_METHOD("set_language", "language"), &FoldableContainer::set_language);
	ClassDB::bind_method(D_METHOD("get_language"), &FoldableContainer::get_language);
	ClassDB::bind_method(D_METHOD("set_text_direction", "text_direction"), &FoldableContainer::set_text_direction);
	ClassDB::bind_method(D_METHOD("get_text_direction"), &FoldableContainer::get_text_direction);
	ClassDB::bind_method(D_METHOD("set_text_overrun_behavior", "overrun_behavior"), &FoldableContainer::set_text_overrun_behavior);
	ClassDB::bind_method(D_METHOD("get_text_overrun_behavior"), &FoldableContainer::get_text_overrun_behavior);
	ClassDB::bind_method(D_METHOD("set_title_position", "title_position"), &FoldableContainer::set_title_position);
	ClassDB::bind_method(D_METHOD("get_title_position"), &FoldableContainer::get_title_position);
	ClassDB::bind_method(D_METHOD("add_button", "icon", "position", "id"), &FoldableContainer::add_button, DEFVAL(-1), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("remove_button", "index"), &FoldableContainer::remove_button);
	ClassDB::bind_method(D_METHOD("get_button_count"), &FoldableContainer::get_button_count);
	ClassDB::bind_method(D_METHOD("get_button_rect", "index"), &FoldableContainer::get_button_rect);
	ClassDB::bind_method(D_METHOD("set_button_id", "index", "id"), &FoldableContainer::set_button_id);
	ClassDB::bind_method(D_METHOD("get_button_id", "index"), &FoldableContainer::get_button_id);
	ClassDB::bind_method(D_METHOD("move_button", "from", "to"), &FoldableContainer::move_button);
	ClassDB::bind_method(D_METHOD("get_button_index", "id"), &FoldableContainer::get_button_index);
	ClassDB::bind_method(D_METHOD("set_button_toggle_mode", "index", "enabled"), &FoldableContainer::set_button_toggle_mode);
	ClassDB::bind_method(D_METHOD("get_button_toggle_mode", "index"), &FoldableContainer::get_button_toggle_mode);
	ClassDB::bind_method(D_METHOD("set_button_toggled", "index", "toggled_on"), &FoldableContainer::set_button_toggled);
	ClassDB::bind_method(D_METHOD("is_button_toggled", "index"), &FoldableContainer::is_button_toggled);
	ClassDB::bind_method(D_METHOD("set_button_icon", "index", "icon"), &FoldableContainer::set_button_icon);
	ClassDB::bind_method(D_METHOD("get_button_icon", "index"), &FoldableContainer::get_button_icon);
	ClassDB::bind_method(D_METHOD("set_button_tooltip", "index", "tooltip"), &FoldableContainer::set_button_tooltip);
	ClassDB::bind_method(D_METHOD("get_button_tooltip", "index"), &FoldableContainer::get_button_tooltip);
	ClassDB::bind_method(D_METHOD("set_button_disabled", "index", "disabled"), &FoldableContainer::set_button_disabled);
	ClassDB::bind_method(D_METHOD("is_button_disabled", "index"), &FoldableContainer::is_button_disabled);
	ClassDB::bind_method(D_METHOD("set_button_hidden", "index", "hidden"), &FoldableContainer::set_button_hidden);
	ClassDB::bind_method(D_METHOD("is_button_hidden", "index"), &FoldableContainer::is_button_hidden);
	ClassDB::bind_method(D_METHOD("set_button_metadata", "index", "metadata"), &FoldableContainer::set_button_metadata);
	ClassDB::bind_method(D_METHOD("get_button_metadata", "index"), &FoldableContainer::get_button_metadata);
	ClassDB::bind_method(D_METHOD("get_button_at_position", "position"), &FoldableContainer::get_button_at_position);

	ADD_SIGNAL(MethodInfo("folding_changed", PropertyInfo(Variant::BOOL, "is_folded")));
	ADD_SIGNAL(MethodInfo("button_pressed", PropertyInfo(Variant::INT, "id"), PropertyInfo(Variant::INT, "index")));
	ADD_SIGNAL(MethodInfo("button_toggled", PropertyInfo(Variant::BOOL, "toggled_on"), PropertyInfo(Variant::INT, "id"), PropertyInfo(Variant::INT, "index")));

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "expanded"), "set_expanded", "is_expanded");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "title_alignment", PROPERTY_HINT_ENUM, "Left,Center,Right"), "set_title_alignment", "get_title_alignment");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "title_position", PROPERTY_HINT_ENUM, "Top,Bottom"), "set_title_position", "get_title_position");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "text_overrun_behavior", PROPERTY_HINT_ENUM, "Trim Nothing,Trim Characters,Trim Words,Ellipsis,Word Ellipsis"), "set_text_overrun_behavior", "get_text_overrun_behavior");

	ADD_GROUP("BiDi", "");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "text_direction", PROPERTY_HINT_ENUM, "Auto,Left-to-Right,Right-to-Left,Inherited"), "set_text_direction", "get_text_direction");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "language", PROPERTY_HINT_LOCALE_ID), "set_language", "get_language");

	BIND_ENUM_CONSTANT(POSITION_TOP);
	BIND_ENUM_CONSTANT(POSITION_BOTTOM);

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, title_style, "title_panel");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, title_hover_style, "title_hover_panel");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, title_collapsed_style, "title_collapsed_panel");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, title_collapsed_hover_style, "title_collapsed_hover_panel");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, focus_style, "focus");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, panel_style, "panel");

	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, button_normal_style);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, button_hovered_style);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, button_pressed_style);
	BIND_THEME_ITEM(Theme::DATA_TYPE_STYLEBOX, FoldableContainer, button_disabled_style);

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_FONT, FoldableContainer, title_font, "font");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_FONT_SIZE, FoldableContainer, title_font_size, "font_size");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_CONSTANT, FoldableContainer, title_font_outline_size, "outline_size");

	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_COLOR, FoldableContainer, title_font_color, "font_color");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_COLOR, FoldableContainer, title_hovered_font_color, "hover_font_color");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_COLOR, FoldableContainer, title_collapsed_font_color, "collapsed_font_color");
	BIND_THEME_ITEM_CUSTOM(Theme::DATA_TYPE_COLOR, FoldableContainer, title_font_outline_color, "font_outline_color");

	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FoldableContainer, button_icon_normal);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FoldableContainer, button_icon_hovered);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FoldableContainer, button_icon_pressed);
	BIND_THEME_ITEM(Theme::DATA_TYPE_COLOR, FoldableContainer, button_icon_disabled);

	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FoldableContainer, arrow);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FoldableContainer, arrow_mirrored);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FoldableContainer, arrow_collapsed);
	BIND_THEME_ITEM(Theme::DATA_TYPE_ICON, FoldableContainer, arrow_collapsed_mirrored);

	BIND_THEME_ITEM(Theme::DATA_TYPE_CONSTANT, FoldableContainer, h_separation);
}

FoldableContainer::FoldableContainer(const String &p_title) {
	text_buf.instantiate();
	set_title(p_title);
	set_focus_mode(FOCUS_ALL);
	set_mouse_filter(MOUSE_FILTER_STOP);
}
