// #pragma once

// #include <algorithm>
// #include <array>
// #include <atomic>
// #include <cassert>
// #include <cstdio>
// #include <functional>
// #include <memory>
// #include <mutex>
// #include <optional>
// #include <string>
// #include <thread>
// #include <vector>

// #include "Core/Stream/Stream.hpp"
// #include "llama.h"

// namespace Translate {
// class MarinaTranslator {
// public:
//   struct Config {
//     std::string model_path = "models/ggml-base.en.bin";
//     bool use_gpu = true;
//     std::string source_language = "ja";
//     std::string target_language = "en";
//   };

//   using TranslationCallback = std::function<void(const std::string&)>;

// public:
//   MarinaTranslator(const Config& config = Config{});
//   ~MarinaTranslator();

//   // Non-copyable
//   MarinaTranslator(const MarinaTranslator&) = delete;
//   MarinaTranslator& operator=(const MarinaTranslator&) = delete;

//   bool Initialize();
//   void Shutdown();

//   // Real-time transcription
//   bool StartRealTimeTranslation(TranslationCallback callback);
//   void StopRealTimeTranslation();
//   bool IsTranslating() const { return m_IsTranslating.load(); }

//   void ProcessText(const std::string& text);
//   const std::string& Translate(const std::string& text);
//   const std::string& Translate(const char* text, int size);

//   // Get configuration
//   const Config& GetConfig() const { return m_Config; }

//   // Model info
//   bool IsModelLoaded() const { return m_Translator.has_value(); }
//   std::vector<std::string> GetSupportedLanguages() const;

// private:
//   struct LlamaTranslator {
//     llama_model* model;
//     llama_context* ctx;
//     const llama_vocab* vocab;
//     llama_sampler* smpl;
//     std::vector<llama_token> prompt_tokens;
//     std::string response;
//     LlamaTranslator(const std::string& model_path, int ngl = 99,
//                     int n_ctx = 2048)
//         : response(1024 * 3, '\0'), prompt_tokens(1024 * 3, 0) {
//       // only print errors
//       // #ifdef _DEBUG
//       //       llama_log_set(
//       //           [](enum ggml_log_level level, const char* text,
//       //              void* /* user_data */) {
//       //             if (level >= GGML_LOG_LEVEL_ERROR) {
//       //               fprintf(stderr, "%s", text);
//       //             }
//       //           },
//       //           nullptr);
//       // #endif

//       // load dynamic backends
//       ggml_backend_load_all();

//       llama_model_params model_params = llama_model_default_params();
//       model_params.n_gpu_layers = ngl;

//       // initialize the model
//       model = llama_model_load_from_file(model_path.c_str(), model_params);
//       if (!model) {
//         fprintf(stderr, "%s: error: unable to load model\n", __func__);
//         return;
//       }

//       vocab = llama_model_get_vocab(model);
//       llama_context_params ctx_params = llama_context_default_params();
//       ctx_params.n_ctx = n_ctx;
//       ctx_params.n_batch = n_ctx;

//       ctx = llama_init_from_model(model, ctx_params);

//       if (!ctx) {
//         fprintf(stderr, "%s: error: failed to create the llama_context\n",
//                 __func__);
//         return;
//       }

//       smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
//       llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0.05f, 1));
//       llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.8f));
//       llama_sampler_chain_add(smpl,
//                               llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
//     }

//     ~LlamaTranslator() {
//       llama_sampler_free(smpl);
//       llama_free(ctx);
//       llama_model_free(model);
//     }

//     const std::string& Generate(const std::string& prompt) {
//       response.clear();

//       const bool is_first =
//           llama_memory_seq_pos_max(llama_get_memory(ctx), 0) == 0;

//       // tokenize the prompt
//       const int n_prompt_tokens = -llama_tokenize(
//           vocab, prompt.c_str(), prompt.size(), NULL, 0, is_first, true);
//       prompt_tokens.resize(n_prompt_tokens);
//       if (llama_tokenize(vocab, prompt.c_str(), prompt.size(),
//                          prompt_tokens.data(), prompt_tokens.size(),
//                          is_first, true) < 0) {
//         GGML_ABORT("failed to tokenize the prompt\n");
//       }

//       // prepare a batch for the prompt
//       llama_batch batch =
//           llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());
//       llama_token new_token_id;
//       while (true) {
//         // check if we have enough space in the context to evaluate this
//         batch int n_ctx = llama_n_ctx(ctx); int n_ctx_used =
//         llama_memory_seq_pos_max(llama_get_memory(ctx), 0); if (n_ctx_used +
//         batch.n_tokens > n_ctx) {
//           fprintf(stderr, "context size exceeded\n");
//           break;
//         }

//         if (llama_decode(ctx, batch)) {
//           GGML_ABORT("failed to decode\n");
//         }

//         // sample the next token
//         new_token_id = llama_sampler_sample(smpl, ctx, -1);

//         // is it an end of generation?
//         if (llama_vocab_is_eog(vocab, new_token_id)) {
//           break;
//         }

//         // convert the token to a string, print it and add it to the response
//         char buf[256];
//         int n = llama_token_to_piece(vocab, new_token_id, buf, sizeof(buf),
//         0,
//                                      true);
//         if (n < 0) {
//           GGML_ABORT("failed to convert token to piece\n");
//         }
//         response.append(buf, n);

//         // prepare the next batch with the sampled token
//         batch = llama_batch_get_one(&new_token_id, 1);
//       }

//       return response;
//     };
//   };
//   Config m_Config;
//   std::optional<LlamaTranslator> m_Translator;
//   std::atomic<bool> m_IsTranslating{false};
//   TranslationCallback m_Callback;
//   std::jthread translationThread;
//   Core::Stream<char> m_Stream{0, 1024 * 3, "MarinaTranslator"};

//   // Internal methods
//   bool LoadModel();
//   void TranslationLoop(std::stop_token s);
// };

// } // namespace Translate