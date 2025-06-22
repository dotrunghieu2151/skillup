#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "Core/Stream/Stream.hpp"
#include "llama.h"

namespace Translate {
class MarinaTranslator {
public:
  struct Config {
    std::string model_path =
        "Assets/LLM/Llama/webbigdata_gemma-2-2b-jpn-it-translate-gguf_gemma-2-"
        "2b-jpn-it-translate-Q4_K_M.gguf";
    bool use_gpu = true;
    std::string source_language = "ja";
    std::string target_language = "en";
  };

  using TranslationCallback = std::function<void(const std::string&)>;

public:
  MarinaTranslator(const Config& config = Config{});
  ~MarinaTranslator();

  // Non-copyable
  MarinaTranslator(const MarinaTranslator&) = delete;
  MarinaTranslator& operator=(const MarinaTranslator&) = delete;

  bool Initialize();
  void Shutdown();

  // Real-time transcription
  bool StartRealTimeTranslation(TranslationCallback callback);
  void StopRealTimeTranslation();
  bool IsTranslating() const { return m_IsTranslating.load(); }

  void ProcessText(const char* text, int size);
  const std::string& Translate(const std::string& text);
  const std::string& Translate(const char* text, int size);

  // Get configuration
  const Config& GetConfig() const { return m_Config; }

  // Model info
  bool IsModelLoaded() const { return m_Translator.has_value(); }
  std::vector<std::string> GetSupportedLanguages() const;

private:
  struct LlamaTranslator {
    llama_model* model;
    llama_context* ctx;
    const llama_vocab* vocab;
    llama_sampler* smpl;
    std::vector<llama_token> prompt_tokens;
    std::string response;
    LlamaTranslator(const std::string& model_path, int ngl = 99,
                    int n_ctx = 2048)
        : response(1024 * 3, '\0'), prompt_tokens(1024 * 3, 0) {
      // only print errors
      // #ifdef _DEBUG
      //       llama_log_set(
      //           [](enum ggml_log_level level, const char* text,
      //              void* /* user_data */) {
      //             if (level >= GGML_LOG_LEVEL_ERROR) {
      //               fprintf(stderr, "%s", text);
      //             }
      //           },
      //           nullptr);
      // #endif

      // load dynamic backends
      ggml_backend_load_all();

      llama_model_params model_params = llama_model_default_params();
      model_params.n_gpu_layers = ngl;

      // initialize the model
      model = llama_model_load_from_file(model_path.c_str(), model_params);
      if (!model) {
        fprintf(stderr, "%s: error: unable to load model\n", __func__);
        return;
      }

      vocab = llama_model_get_vocab(model);
      llama_context_params ctx_params = llama_context_default_params();
      ctx_params.n_ctx = n_ctx;
      ctx_params.n_batch = n_ctx;

      ctx = llama_init_from_model(model, ctx_params);

      if (!ctx) {
        fprintf(stderr, "%s: error: failed to create the llama_context\n",
                __func__);
        return;
      }

      smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
      llama_sampler_chain_add(smpl, llama_sampler_init_greedy());
      // llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0.05f, 1));
      // llama_sampler_chain_add(smpl, llama_sampler_init_temp(0.8f));
      // llama_sampler_chain_add(smpl,
      //                         llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    }

    ~LlamaTranslator() {
      llama_sampler_free(smpl);
      llama_free(ctx);
      llama_model_free(model);
    }

    const std::string& Generate(const char* text, int size) {
      if (!model || !ctx || !vocab || !smpl) {
        response.clear();
        return response;
      }

      // Skip empty or very short inputs
      if (size <= 0 || (size == 1 && text[0] == '\0')) {
        response.clear();
        return response;
      }

      // Create efficient translation prompt for Japanese->English
      std::string input_text(text, size);

      // Remove trailing nulls and whitespace for cleaner processing
      if (input_text.empty()) {
        response.clear();
        return response;
      }

      // Optimized prompt for Gemma-2-2b Japanese translation model
      std::string prompt =
          "<start_of_turn>user\nTranslate this Japanese text to English: " +
          input_text + "<end_of_turn>\n<start_of_turn>model\n";

      // Tokenize the prompt - get count first
      const int n_tokens_check = -llama_tokenize(
          vocab, prompt.c_str(), prompt.size(), nullptr, 0, true, true);
      if (n_tokens_check <= 0) {
        response.clear();
        return response;
      }

      prompt_tokens.resize(n_tokens_check);
      const int n_tokens = llama_tokenize(vocab, prompt.c_str(),
                                          prompt.length(), prompt_tokens.data(),
                                          prompt_tokens.size(), true, true);

      if (n_tokens < 0 || n_tokens != n_tokens_check) {
        response.clear();
        return response;
      }

      // Clear previous context and evaluate prompt
      llama_memory_t memory = llama_get_memory(ctx);
      llama_memory_clear(memory, false);

      llama_batch batch =
          llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

      // Generate response tokens - optimized for real-time
      response.clear();

      const int max_tokens =
          1024; // Limit response length for real-time performance
      int n_decoded = 0;
      llama_token new_token_id;
      while (n_decoded < max_tokens) {

        if (llama_decode(ctx, batch)) {
          break;
        }
        // Sample next token
        new_token_id = llama_sampler_sample(smpl, ctx, -1);

        // Check for end of sequence
        if (llama_vocab_is_eog(vocab, new_token_id)) {
          break;
        }

        // Decode token to text
        char token_str[1024];
        int token_len = llama_token_to_piece(vocab, new_token_id, token_str,
                                             sizeof(token_str), 0, true);

        if (token_len > 0) {
          response.append(token_str, token_len);
        }

        // Prepare for next iteration
        batch = llama_batch_get_one(&new_token_id, 1);

        ++n_decoded;

        // Early termination for real-time performance
        // Stop if we hit natural sentence boundaries
      }

      // Remove common model artifacts that might appear
      return response;
    }
  };
  Config m_Config;
  std::optional<LlamaTranslator> m_Translator;
  std::atomic<bool> m_IsTranslating{false};
  TranslationCallback m_Callback;
  std::jthread translationThread;
  Core::Stream<char> m_Stream{0, 1024 * 3, "MarinaTranslator"};

  // Internal methods
  bool LoadModel();
  void TranslationLoop(std::stop_token s);
};

} // namespace Translate