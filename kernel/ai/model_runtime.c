/* Minimal AI model runtime for EMBODIOS */

/* Kernel AI runtime - using integer-only inference for ARM64 compatibility */

#include "embodios/types.h"
#include "embodios/kernel.h"
#include "embodios/console.h"

/* Model runtime state */
typedef struct {
    struct embody_model *model;
    void *workspace;
    size_t workspace_size;
    bool initialized;
} model_runtime_t;

static model_runtime_t runtime = {
    .model = NULL,
    .workspace = NULL,
    .workspace_size = 0,
    .initialized = false
};

/* Initialize model runtime.
 * The legacy EMBODIOS-format / TVM graph-executor API (model_load,
 * model_inference, inference_run, ...) was removed with the duplicate
 * engines in ai/_archive/. GGUF inference runs through the canonical
 * streaming path: gguf_parser.c -> bpe_tokenizer.c -> streaming_inference.c. */
void model_runtime_init(void)
{
    runtime.initialized = true;
    console_printf("AI Runtime: Initialized\n");
}
