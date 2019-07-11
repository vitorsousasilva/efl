#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Efl_Ui.h>
#include <Elementary.h>
#include "efl_ui_suite.h"

static unsigned int event_counter;

static void
slider_changed(void *data EINA_UNUSED, const Efl_Event *ev)
{
   event_counter++;
   if (event_counter == 1)
     efl_event_callback_del(ev->object, EFL_UI_SLIDER_EVENT_CHANGED, slider_changed, NULL);
   else if (event_counter == 2)
     ecore_main_loop_quit();
}

EFL_START_TEST(efl_ui_test_slider_events)
{
   Eo *slider;
   Evas *e;
   Eo *win = win_add();

   efl_gfx_entity_size_set(win, EINA_SIZE2D(400, 100));
   slider = efl_add(EFL_UI_SLIDER_CLASS, win,
                efl_event_callback_add(efl_added, EFL_UI_SLIDER_EVENT_CHANGED, slider_changed, NULL),
                efl_event_callback_add(efl_added, EFL_UI_SLIDER_EVENT_STEADY, slider_changed, NULL),
                efl_gfx_entity_size_set(efl_added, EINA_SIZE2D(400, 100))
                );

   e = evas_object_evas_get(win);

   efl_layout_signal_process(slider, EINA_TRUE);
   get_me_to_those_events(slider);


   int x, y, w, h;
   int sx, sy, sw, sh;

   evas_object_geometry_get(elm_object_part_content_get(slider, "efl.bar"), &x, &y, &w, &h);
   evas_object_geometry_get(slider, &sx, &sy, &sw, &sh);
   evas_event_feed_mouse_in(e, 0, NULL);
   evas_event_feed_mouse_move(e, x + (w / 2), y + (h / 2), 0, NULL);
   evas_event_feed_mouse_down(e, 1, 0, 0, NULL);
   evas_event_feed_mouse_move(e, sx + (sw / 2), sy + (sh / 2), 0, NULL);
   evas_event_feed_mouse_up(e, 1, 0, 0, NULL);
   ecore_main_loop_begin();
   ck_assert_int_eq(event_counter, 2);
}
EFL_END_TEST

void efl_ui_test_slider(TCase *tc)
{
   tcase_add_test(tc, efl_ui_test_slider_events);
}
