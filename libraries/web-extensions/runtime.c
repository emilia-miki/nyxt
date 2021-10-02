#include "globals.h"
#include "extevent.h"
#include "runtime.h"

static unsigned long int
runtime_send_message_callback (char *extension_id, JSCValue *object)
{
        JSCContext *context = jsc_context_get_current();
        JSCValue *wrapper = jsc_value_new_object(context, NULL, NULL);
        jsc_value_object_set_property(
                wrapper, "extensionId",
                jsc_value_new_string(context, extension_id));
        jsc_value_object_set_property(wrapper, "message", object);
        char *json = jsc_value_to_json(wrapper, 0);
        GVariant *variant = g_variant_new("ms", json);
        WebKitUserMessage *message = webkit_user_message_new("runtime.sendMessage", variant);
        SEND_MESSAGE_RETURN_ID(message, i);
}

static JSCValue *
runtime_get_manifest_callback (char *extension_name)
{
        ExtensionData *data = g_hash_table_lookup(EXTENSIONS_DATA, extension_name);
        JSCContext *context = jsc_context_get_current();
        return jsc_value_new_from_json(context, jsc_value_to_json(data->manifest, 0));
}

static unsigned long int
runtime_get_platform_info_callback ()
{
        WebKitUserMessage *message = webkit_user_message_new("runtime.getPlatformInfo", NULL);
        SEND_MESSAGE_RETURN_ID(message, i);
}

static unsigned long int
runtime_get_browser_info_callback ()
{
        WebKitUserMessage *message = webkit_user_message_new("runtime.getBrowserInfo", NULL);
        SEND_MESSAGE_RETURN_ID(message, i);
}

static char *
runtime_get_url_callback (char *extension_name, char *path)
{
        JSCContext *context = jsc_context_get_current();
        char *result = malloc(sizeof(char) * 4000000000); /* 4GB */
        result = "data:text/html,<h1>Resource not found</h1>";
        ExtensionData *data = g_hash_table_lookup(EXTENSIONS_DATA, extension_name);
        if (!data)
                return result;
        JSCValue *files = data->files;
        char **properties = jsc_value_object_enumerate_properties(files);
        char **property;
        if (properties)
                for (property = properties; *property != NULL; property++)
                        if (!strcmp(*property, path))
                                result = jsc_value_to_string(jsc_value_object_get_property(files, *property));
        return result;
}

void inject_runtime_api (char* extension_name)
{
        JSCContext *context = get_extension_context(IS_PRIVILEGED ? NULL : extension_name);
        MAKE_CLASS(context, Runtime, "runtime");

        MAKE_FN(context, runtimeSendMessage, runtime_send_message_callback, G_TYPE_ULONG, 2, G_TYPE_STRING, JSC_TYPE_VALUE);
        MAKE_FN(context, runtimeGetManifest, runtime_get_manifest_callback, JSC_TYPE_VALUE, 1, G_TYPE_STRING);
        MAKE_FN(context, runtimeGetPlatformInfo, runtime_get_platform_info_callback, G_TYPE_ULONG, 0, G_TYPE_NONE);
        MAKE_FN(context, runtimeGetBrowserInfo, runtime_get_browser_info_callback, G_TYPE_ULONG, 0, G_TYPE_NONE);
        MAKE_FN(context, runtimeGetURL, runtime_get_url_callback, G_TYPE_STRING, 2, G_TYPE_STRING, G_TYPE_STRING);

        MAKE_EVENT(context, "runtime", "onMessage");

        BIND_FN(context, "runtime", "sendMessage", "runtime.sendMessage = function (one, two, three) {\
    var no_two = (two === undefined || two === null ||                  \
                  (two.hasOwnProperty(\"includeTlsChannelId\") &&       \
                   two.keys.length <= 1));                              \
    var no_three = (three === undefined);                               \
    return new Promise(function (success, failure) {                    \
        try {                                                           \
            var message = (no_two && no_three) ? one : two;             \
            var extensionId = (no_two && no_three) ? runtime.id : one;  \
            browser.drain(runtimeSendMessage(extensionId, message), success, undefined, 5000); \
        } catch (error) {                                               \
            return failure(error);                                      \
        };                                                              \
    });                                                                 \
};                                                                      \
                                                                        \
runtime.sendMessage");
        BIND_FN(context, "runtime", "getPlatformInfo", "runtime.getPlatformInfo = function() {\
    return new Promise ((success, failure) => {                         \
        try {                                                           \
            browser.drain(runtimeGetPlatformInfo(), success, {}, 5000); \
        } catch (error) {                                               \
            return failure(error);                                      \
        };                                                              \
    });                                                                 \
};                                                                      \
                                                                        \
runtime.getPlatformInfo");
        BIND_FN(context, "runtime", "getBrowserInfo", "runtime.getBrowserInfo = function() {\
    return new Promise ((success, failure) => {                         \
        try {                                                           \
            browser.drain(runtimeGetBrowserInfo(), success, {}, 5000);\
        } catch (error) {                                               \
            return failure(error);                                      \
        };                                                              \
    });                                                                 \
};                                                                      \
                                                                        \
runtime.getBrowserInfo");

        char *runtime_get_manifest_js = malloc(sizeof(char) * 300);
        sprintf(runtime_get_manifest_js, "runtime.getManifest = function () { \
    return runtimeGetManifest(\"%s\");                                  \
};                                                                      \
                                                                        \
runtime.getManifest", extension_name);
        BIND_FN(context, "runtime", "getURL", "runtime.getURL = function (string) {\
    return runtimeGetURL(runtime.name, (string[0] === '/' ? string : '/' + string)); \
};                                                                      \
                                                                        \
runtime.getURL");
        jsc_value_object_set_property(
                jsc_context_evaluate(context, "runtime", -1),
                "getManifest",
                jsc_context_evaluate(context, runtime_get_manifest_js, -1));
        jsc_value_object_set_property(
                jsc_context_evaluate(context, "runtime", -1),
                "id",
                jsc_value_new_string(context, get_extension_id(extension_name)));
        jsc_value_object_set_property(
                jsc_context_evaluate(context, "runtime", -1),
                "name",
                jsc_value_new_string(context, extension_name));
        jsc_value_object_set_property(
                jsc_context_evaluate(context, "browser", -1), "runtime",
                jsc_context_evaluate(context, "runtime", -1));
        free(runtime_get_manifest_js);
}