#include "m-array.h"
#include "m-string.h"

// Define an array of string_t. Since the oplist of string_t is registered globaly
// there is no need to give it to this definition.
M_ARRAY_DEF(str_array, string_t)

// First the verbose way and (nearly) macro free:
static void main_macrofree(void)
{
    str_array_t tab_name;
    str_array_init(tab_name);

    // Push some elements in the array (long way)
    str_array_push_back(tab_name, STRING_CTE("My"));
    str_array_push_back(tab_name, STRING_CTE("CD"));
    str_array_push_back(tab_name, STRING_CTE("IS"));
    str_array_push_back(tab_name, STRING_CTE("OUT"));

    // Overwrite element 1 of the array.
    str_array_set_at(tab_name, 1, STRING_CTE("DVD"));

    // Format some strings and push them back in the array
    string_t format;
    string_init_printf(format, "There are %d elements", str_array_size(tab_name));
    str_array_push_back(tab_name, format);
    M_F(string, printf)(format, "There is a capacty of %d", str_array_capacity(tab_name));
    M_F(string, replace_all, cstr)(format, "capacty", "capacity");
    str_array_push_back(tab_name, format);
    M_F(string, printf)(format, "The third element is '%s'", M_F(string, get_cstr)(*str_array_get(tab_name, 2)));
    str_array_push_back(tab_name, format);
    string_clear(format);

    // Display the content of the array.
    str_array_it_t it;
    int i = 0;
    for (str_array_it(it, tab_name) ; !str_array_end_p(it); str_array_next(it)) {
        printf("item[%d] = '%s'\n", i, M_F(string, get_cstr)(*str_array_cref(it)) );
        i++;
    }

    // Clear the array
    str_array_clear(tab_name);
}

// Then the short version with macros. 
// For this we need to register the oplist of the array of string globally.
#define M_OPL_str_array_t() M_ARRAY_OPLIST(str_array, STRING_OPLIST)

char pwd[256];

static void main_macro(void)
{
    // Define and fill an array of string with some elements in it:
    M_LET( (tab_name, STRING_CTE("My"), STRING_CTE("CD"), STRING_CTE("IS"), STRING_CTE("OUT")), str_array_t) {
        // Overwrite element 1 of the array.
        str_array_set_at(tab_name, 1, STRING_CTE("DVD"));

        // Format some strings and push them back in the array
        M_LET(format, string_t) {
            M_F(string, printf)(format, "There are %d elements", str_array_size(tab_name));
            str_array_push_back(tab_name, format);
            M_F(string, printf)(format, "There is a capacty of %d", str_array_capacity(tab_name));
            M_F(string, replace_all, cstr)(format, "capacty", "capacity");
            str_array_push_back(tab_name, format);
            M_F(string, printf)(format, "The third element is '%s'", M_F(string, get_cstr)(*str_array_get(tab_name, 2)));
            str_array_push_back(tab_name, format);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
            // Set format to a formated string
            // In C11, we can mix string_t and char *.
            // We can also use M_CSTR to create a printf formated string.
            size_t pwd_size;
            m_core_getenv_s(&pwd_size, pwd, sizeof(pwd), "PWD");
            string_sets(format, "FILE=", pwd, "/", *str_array_get(tab_name, 2), M_CSTR("-%d.txt", str_array_size(tab_name) ));
            str_array_push_back(tab_name, format);
#endif
        } // beyond this point format is cleared

        // Display the content of the array.
        int i = 0;
        for M_EACH(item, tab_name, str_array_t) {
            printf("item[%d] = '%s'\n", i, M_F(string, get_cstr)(*item) );
            i++;
        }
    } // beyond this point tab_name is cleared
}

int main(void)
{
    printf("Version macrofree:\n");
    main_macrofree();
    printf("Version macro:\n");
    main_macro();
    return 0;
}
