#include <Eina.h>
#include <string.h>

#include "Eolian.h"
#include "types_generator.h"
#include "common_funcs.h"

static char *
_concat_name(const Eolian_Type *tp)
{
   const char *name;
   char *str = NULL;
   Eina_Strbuf *buf = eina_strbuf_new();
   Eina_Iterator *itr = eolian_type_namespaces_get(tp);
   EINA_ITERATOR_FOREACH(itr, name)
      if (name) eina_strbuf_append_printf(buf, "%s_", name);
   eina_iterator_free(itr);
   name = eolian_type_name_get(tp);
   if (name) eina_strbuf_append_printf(buf, "%s", name);
   if (eina_strbuf_length_get(buf))
      str = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);
   return str;
}

static void
_desc_generate(const char *desc, Eina_Strbuf *buf)
{
   if (desc)
     {
        eina_strbuf_append(buf, "/**\n");
        eina_strbuf_append(buf, desc);
        eina_strbuf_replace_all(buf, "\n", "\n * ");
        eina_strbuf_append(buf, "\n */\n");
        eina_strbuf_replace_all(buf, " * \n", " *\n"); /* Remove trailing whitespaces */
     }
}

static Eina_Strbuf *
_type_generate(const Eolian_Type *tp, Eina_Bool in_typedef)
{
   Eina_Strbuf *buf = eina_strbuf_new();
   _desc_generate(eolian_type_description_get(tp), buf);
   Eolian_Type_Type tp_type = eolian_type_type_get(tp);
   switch(tp_type)
     {
      case EOLIAN_TYPE_ALIAS:
           {
              const Eolian_Type *base_tp = eolian_type_base_type_get(tp);
              Eolian_Type_Type base_tp_type = eolian_type_type_get(base_tp);
              if (base_tp_type == EOLIAN_TYPE_STRUCT || base_tp_type == EOLIAN_TYPE_ENUM)
                {
                   const char *name = eolian_type_name_get(tp);
                   Eina_Strbuf *struct_buf = _type_generate(base_tp, EINA_TRUE);
                   eina_strbuf_append_printf(buf, "typedef %s%s%s",
                         eina_strbuf_string_get(struct_buf),
                         name?" ":"", name?name:"");
                   eina_strbuf_free(struct_buf);
                }
              else
                {
                   char *name = _concat_name(tp);
                   Eina_Stringshare *c_type = eolian_type_c_type_get(base_tp);
                   eina_strbuf_append_printf(buf, "typedef %s%s%s",
                         c_type, !name || strchr(c_type, '*')?"":" ",
                         name?name:"");
                   free(name);
                }
              break;
           }
      case EOLIAN_TYPE_STRUCT:
           {
              const char *member_name;
              char *name = _concat_name(tp);
              if (in_typedef && name)
                {
                   eina_strbuf_append_printf(buf, "struct %s", name);
                   free(name);
                   break;
                }
              eina_strbuf_append_printf(buf, "struct%s%s {\n", name?" ":"", name?name:"");
              free(name);
              Eina_Iterator *members = eolian_type_struct_field_names_get(tp);
              EINA_ITERATOR_FOREACH(members, member_name)
                {
                   const char *desc = eolian_type_struct_field_description_get(tp, member_name);
                   const Eolian_Type *member = eolian_type_struct_field_get(tp, member_name);
                   Eina_Stringshare *c_type = eolian_type_c_type_get(member);
                   eina_strbuf_append_printf(buf, "  %s%s%s;",
                         c_type, strchr(c_type, '*')?"":" ",
                         member_name);
                   if (desc) eina_strbuf_append_printf(buf, " /** %s */", desc);
                   eina_strbuf_append(buf, "\n");
                }
              eina_iterator_free(members);
              eina_strbuf_append(buf, "}");
              break;
           }
      case EOLIAN_TYPE_ENUM:
           {
              const char *member_name;
              char *name = _concat_name(tp);
              if (in_typedef)
                {
                   eina_strbuf_append_printf(buf, "enum %s", name);
                   free(name);
                   break;
                }
              char *pre = NULL;
              eina_strbuf_append_printf(buf, "enum %s {\n", name);
              if (eolian_type_enum_legacy_prefix_get(tp))
                {
                   pre = strdup(eolian_type_enum_legacy_prefix_get(tp));
                   free(name);
                }
              else
                pre = name;
              eina_str_toupper(&pre);
              Eina_Iterator *members = eolian_type_enum_field_names_get(tp);
              Eina_Bool next = eina_iterator_next(members, (void**)&member_name);
              Eina_Strbuf *membuf = eina_strbuf_new();
              while (next)
                {
                   const char *desc = eolian_type_enum_field_description_get(tp, member_name);
                   const Eolian_Expression *member = eolian_type_enum_field_get(tp, member_name);
                   char *memb_u = strdup(member_name);
                   eina_str_toupper(&memb_u);
                   eina_strbuf_reset(membuf);
                   eina_strbuf_append(membuf, pre);
                   eina_strbuf_append_char(membuf, '_');
                   eina_strbuf_append(membuf, memb_u);
                   free(memb_u);
                   if (!member)
                     eina_strbuf_append_printf(buf, "  %s", eina_strbuf_string_get(membuf));
                   else
                     {
                        Eina_Value *v = NULL;
                        Eolian_Expression_Type et = eolian_expression_eval(member, EOLIAN_MASK_INT, &v);
                        const char *lit = eolian_expression_value_to_literal(v, et);
                        eina_strbuf_append_printf(buf, "  %s = %s", eina_strbuf_string_get(membuf), lit);
                        eina_stringshare_del(lit);
                     }
                   next = eina_iterator_next(members, (void**)&member_name);
                   if (next)
                     eina_strbuf_append(buf, ",");
                   if (desc) eina_strbuf_append_printf(buf, " /** %s */", desc);
                   eina_strbuf_append(buf, "\n");
                }
              eina_strbuf_free(membuf);
              free(pre);
              eina_iterator_free(members);
              eina_strbuf_append(buf, "}");
              break;
           }
      default:
           {
              eina_strbuf_reset(buf);
           }
     }
   return buf;
}

Eina_Bool
types_header_generate(const char *eo_filename, Eina_Strbuf *buf)
{
   const Eolian_Type *tp;

   /* Generation of typedefs */
   Eina_Iterator *itr = eolian_type_aliases_get_by_file(eo_filename);
   EINA_ITERATOR_FOREACH(itr, tp)
     {
        Eina_Strbuf *type_buf = _type_generate(tp, EINA_TRUE);
        if (type_buf)
          {
             eina_strbuf_append(buf, eina_strbuf_string_get(type_buf));
             eina_strbuf_append(buf, ";\n\n");
             eina_strbuf_free(type_buf);
          }
     }
   eina_iterator_free(itr);

   /* Generation of structs */
   itr = eolian_type_structs_get_by_file(eo_filename);
   EINA_ITERATOR_FOREACH(itr, tp)
     {
        Eina_Strbuf *type_buf = _type_generate(tp, EINA_FALSE);
        if (type_buf)
          {
             eina_strbuf_append(buf, eina_strbuf_string_get(type_buf));
             eina_strbuf_append(buf, ";\n\n");
             eina_strbuf_free(type_buf);
          }
     }
   eina_iterator_free(itr);

   /* Generation of enums */
   itr = eolian_type_enums_get_by_file(eo_filename);
   EINA_ITERATOR_FOREACH(itr, tp)
     {
        Eina_Strbuf *type_buf = _type_generate(tp, EINA_FALSE);
        if (type_buf)
          {
             eina_strbuf_append(buf, eina_strbuf_string_get(type_buf));
             eina_strbuf_append(buf, ";\n\n");
             eina_strbuf_free(type_buf);
          }
     }
   eina_iterator_free(itr);

   return EINA_TRUE;
}

Eina_Bool
types_class_typedef_generate(const char *eo_filename, Eina_Strbuf *buf)
{
   const Eolian_Class *class = eolian_class_get_by_file(eo_filename);
   if (!class)
     return EINA_FALSE;

   static _eolian_class_vars class_env;
   _class_env_create(class, NULL, &class_env);

   eina_strbuf_append_printf(buf, "typedef Eo %s;\n", class_env.full_classname);
   return EINA_TRUE;
}
