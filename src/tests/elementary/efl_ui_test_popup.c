#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include <Elementary.h>
#include "efl_ui_suite.h"

#define WIN_SIZE 500

EFL_START_TEST(efl_ui_test_popup_basic_align)
{
   Eo *win, *popup;

   win = win_add();
   efl_gfx_entity_size_set(win, EINA_SIZE2D(WIN_SIZE, WIN_SIZE));

   popup = efl_add(EFL_UI_POPUP_CLASS, win);
   efl_ui_popup_size_set(popup, EINA_SIZE2D(160, 160));

   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, popup);
   efl_text_set(btn, "Efl.Ui.Popup");

   efl_content_set(popup, btn);


   get_me_to_those_events(popup);

   {
      Eina_Position2D pos = efl_gfx_entity_position_get(popup);
      Eina_Size2D sz = efl_gfx_entity_size_get(popup);

      /* verify centered */
      ck_assert_int_eq(pos.x, WIN_SIZE / 2 - sz.w / 2);
      ck_assert_int_eq(pos.y, WIN_SIZE / 2 - sz.h / 2);
   }

   efl_ui_popup_align_set(popup, EFL_UI_POPUP_ALIGN_LEFT);
   efl_canvas_group_calculate(popup);
   {
      Eina_Position2D pos = efl_gfx_entity_position_get(popup);
      Eina_Size2D sz = efl_gfx_entity_size_get(popup);

      /* verify left */
      ck_assert_int_eq(pos.x, 0);
      ck_assert_int_eq(pos.y, WIN_SIZE / 2 - sz.h / 2);
   }


   efl_ui_popup_align_set(popup, EFL_UI_POPUP_ALIGN_RIGHT);
   efl_canvas_group_calculate(popup);
   {
      Eina_Position2D pos = efl_gfx_entity_position_get(popup);
      Eina_Size2D sz = efl_gfx_entity_size_get(popup);

      /* verify right */
      ck_assert_int_eq(pos.x, WIN_SIZE - sz.w);
      ck_assert_int_eq(pos.y, WIN_SIZE / 2 - sz.h / 2);
   }

   efl_ui_popup_align_set(popup, EFL_UI_POPUP_ALIGN_TOP);
   efl_canvas_group_calculate(popup);
   {
      Eina_Position2D pos = efl_gfx_entity_position_get(popup);
      Eina_Size2D sz = efl_gfx_entity_size_get(popup);

      /* verify top */
      ck_assert_int_eq(pos.x, WIN_SIZE / 2 - sz.w / 2);
      ck_assert_int_eq(pos.y, 0);
   }

   efl_ui_popup_align_set(popup, EFL_UI_POPUP_ALIGN_BOTTOM);
   efl_canvas_group_calculate(popup);
   {
      Eina_Position2D pos = efl_gfx_entity_position_get(popup);
      Eina_Size2D sz = efl_gfx_entity_size_get(popup);

      /* verify bottom */
      ck_assert_int_eq(pos.x, WIN_SIZE / 2 - sz.w / 2);
      ck_assert_int_eq(pos.y, WIN_SIZE - sz.h);
   }

   efl_gfx_entity_position_set(popup, EINA_POSITION2D(0, 0));
   efl_canvas_group_calculate(popup);
   {
      Eina_Position2D pos = efl_gfx_entity_position_get(popup);

      /* verify bottom */
      ck_assert_int_eq(pos.x, 0);
      ck_assert_int_eq(pos.y, 0);
   }
}
EFL_END_TEST

EFL_START_TEST(efl_ui_test_popup_basic_sizing)
{
   Eo *win, *popup;

   win = win_add();
   efl_gfx_entity_size_set(win, EINA_SIZE2D(WIN_SIZE, WIN_SIZE));

   popup = efl_add(EFL_UI_POPUP_CLASS, win);
   efl_ui_popup_size_set(popup, EINA_SIZE2D(160, 160));

   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, popup);
   efl_text_set(btn, "Efl.Ui.Popup");

   efl_content_set(popup, btn);


   get_me_to_those_events(popup);

   Eina_Size2D sz = efl_gfx_entity_size_get(popup);
   ck_assert_int_eq(sz.w, 160);
   ck_assert_int_eq(sz.h, 160);
}
EFL_END_TEST

EFL_START_TEST(efl_ui_test_popup_events)
{
   Eo *win, *popup;
   int called = 0;

   win = win_add();
   efl_gfx_entity_size_set(win, EINA_SIZE2D(WIN_SIZE, WIN_SIZE));

   popup = efl_add(EFL_UI_POPUP_CLASS, win);

   efl_event_callback_add(popup, EFL_UI_POPUP_EVENT_BACKWALL_CLICKED,
     (void*)event_callback_that_is_called_exactly_one_time_and_sets_a_single_int_data_pointer_when_called, &called);
   efl_event_callback_add(popup, EFL_UI_POPUP_EVENT_TIMEOUT, event_callback_that_quits_the_main_loop_when_called, NULL);
   efl_ui_popup_size_set(popup, EINA_SIZE2D(160, 160));

   Eo *btn = efl_add(EFL_UI_BUTTON_CLASS, popup);
   efl_text_set(btn, "Efl.Ui.Popup");

   efl_content_set(popup, btn);


   get_me_to_those_events(popup);
   click_object_at(win, 1, 1);
   /* trigger signal process -> events */
   ecore_main_loop_iterate();
   ck_assert_int_eq(called, 1);
   called = 0;

   Eo *repeat_test_btn = efl_add(EFL_UI_BUTTON_CLASS, win);
   efl_gfx_entity_position_set(repeat_test_btn, EINA_POSITION2D(0, 0));
   efl_gfx_entity_size_set(repeat_test_btn, EINA_SIZE2D(100, 100));
   efl_text_set(repeat_test_btn, "Repeat Event Test");
   efl_event_callback_add(repeat_test_btn, EFL_INPUT_EVENT_CLICKED,
     (void*)event_callback_that_is_called_exactly_one_time_and_sets_a_single_int_data_pointer_when_called, &called);
   efl_ui_popup_part_backwall_repeat_events_set(efl_part(popup, "backwall"), EINA_TRUE);
   click_object(repeat_test_btn);

   efl_ui_popup_timeout_set(popup, 0.1);
   ecore_main_loop_begin();
}
EFL_END_TEST

EFL_START_TEST(efl_ui_test_popup_backwall_img)
{
   Eo *win, *popup;
   char buf[PATH_MAX];

   win = win_add();
   efl_gfx_entity_size_set(win, EINA_SIZE2D(WIN_SIZE, WIN_SIZE));

   popup = efl_add(EFL_UI_POPUP_CLASS, win);

   snprintf(buf, sizeof(buf), "%s/images/sky_01.jpg", ELM_IMAGE_DATA_DIR);
   ck_assert(efl_file_simple_load(efl_part(popup, "backwall"), buf, NULL));
   ck_assert_str_eq(efl_file_get(efl_part(popup, "backwall")), buf);
}
EFL_END_TEST

void efl_ui_test_popup(TCase *tc)
{
   tcase_add_test(tc, efl_ui_test_popup_events);
   tcase_add_test(tc, efl_ui_test_popup_basic_sizing);
   tcase_add_test(tc, efl_ui_test_popup_basic_align);
   tcase_add_test(tc, efl_ui_test_popup_backwall_img);
}