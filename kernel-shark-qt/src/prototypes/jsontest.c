#include <stdio.h>
#include <stdlib.h>
#include <json.h>

#include "libkshark.h"
#include "libkshark-json.h"


// 	printf(" %s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

int main(int argc, char **argv)
{
	struct kshark_context *kshark_ctx = NULL;
	kshark_instance(&kshark_ctx);

// 	filter_id_add(kshark_ctx->show_task_filter, 314);
// 	filter_id_add(kshark_ctx->show_task_filter, 42);
// 	filter_id_add(kshark_ctx->hide_task_filter, 666);
// 	filter_id_add(kshark_ctx->hide_event_filter, 1234);
// 	
// 	struct json_object *jobj = kshark_all_filters_to_json(kshark_ctx);
// 	kshark_save_json_file("filterconfig.json", jobj);
// 	json_object_put(jobj);

	struct json_object *jobj =
		json_object_from_file("filter1.json");
	
// 	printf(" %s\n", json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));
// 
	struct json_object *tmp;
	json_object_object_get_ex(jobj, "type", &tmp);
// 	printf(" %s\n", json_object_to_json_string(tmp));

	struct json_object *f1;
	json_object_object_get_ex(jobj, "show event filter", &f1);
// 	json_object_object_get_ex(jobj, "hide event filter", &f1);
// 	printf(" %s\n", json_object_to_json_string_ext(f1, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY));

	json_object_put(jobj);
	kshark_free(kshark_ctx);
	return 0;
}


