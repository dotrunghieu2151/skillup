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
        "Assets/LLM/aya-23-8B-Q4_K_M.gguf"; // Update this to your Aya 23 8B
                                            // model path
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

  // Context management
  void ClearTranslationHistory() {
    if (m_Translator.has_value()) {
      m_Translator.value().ClearConversationHistory();
    }
  }

private:
  struct LlamaTranslator {
    llama_model* model;
    llama_context* ctx;
    const llama_vocab* vocab;
    llama_sampler* smpl;
    std::vector<llama_token> prompt_tokens;
    std::vector<llama_chat_message> chat_messages;
    std::vector<char> formatted;
    std::string response;
    bool conversation_initialized = false;
    int max_context_pairs = 5;   // Keep last 5 translation pairs for context
    int max_batch_tokens = 1800; // Leave room under n_batch limit for safety

    // Static helper function for creating translation prompts optimized for
    // Qwen3
    static std::string create_translation_prompt() {
      return "/no_think You are an expert translator specializing in Japanese "
             "to English translation. "
             "Your task is to provide accurate, natural, and culturally "
             "appropriate translations while maintaining consistency with "
             "previous translations.\n\n"
             "Guidelines:\n"
             "- Translate the Japanese text into fluent, natural English\n"
             "- Preserve the original meaning, tone, and intent\n"
             "- Maintain appropriate formality levels\n"
             "- Handle cultural references and idioms appropriately\n"
             "- Use previous translations as context for consistency\n"
             "- Maintain speaker identity and conversation flow\n"
             "- Keep consistent terminology and style throughout\n"
             "- Provide only the English translation without explanations\n"
             "- Do not show your thinking process\n\n"
             "Important: This is part of an ongoing conversation. Use the "
             "previous Japanese-English pairs to understand context, maintain "
             "consistency, and provide coherent translations.\n\n";
    }

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
      ctx_params.n_ctx = 4096; // Larger context for Japanese
      ctx_params.n_batch =
          2048; // Increased batch size to handle conversation history

      ctx = llama_init_from_model(model, ctx_params);

      if (!ctx) {
        fprintf(stderr, "%s: error: failed to create the llama_context\n",
                __func__);
        return;
      }

      // Optimized sampling parameters for Qwen3 non-thinking mode
      smpl = llama_sampler_chain_init(llama_sampler_chain_default_params());
      llama_sampler_chain_add(smpl, llama_sampler_init_min_p(0.0f, 1));
      llama_sampler_chain_add(
          smpl, llama_sampler_init_top_p(0.8f, 1)); // Model card recommendation
      llama_sampler_chain_add(
          smpl, llama_sampler_init_temp(0.7f)); // Model card recommendation
      llama_sampler_chain_add(
          smpl, llama_sampler_init_top_k(20)); // Model card recommendation
      llama_sampler_chain_add(smpl,
                              llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
      formatted.resize(llama_n_ctx(ctx));
    }

    ~LlamaTranslator() {
      // Clean up strdup'd chat message content
      for (auto& msg : chat_messages) {
        free((void*)msg.content);
      }
      chat_messages.clear();

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
      std::printf("Input text for translation: %s\n", input_text.c_str());

      // Remove trailing nulls and whitespace for cleaner processing
      if (input_text.empty()) {
        response.clear();
        return response;
      }

      // Create proper prompt with system message and user message
      std::string system_prompt = create_translation_prompt();
      std::string user_message =
          "/no_think Translate this Japanese text to English: " + input_text;

      // Get the model's chat template
      const char* tmpl = llama_model_chat_template(model, nullptr);

      // Initialize conversation with system prompt only once
      if (!conversation_initialized) {
        chat_messages.clear();
        chat_messages.push_back({"system", strdup(system_prompt.c_str())});
        conversation_initialized = true;
      }

      // Manage context window using token-aware approach
      // First try to estimate current token count
      int estimated_tokens = EstimateTokenCount();
      int pair_count = (chat_messages.size() - 1) / 2; // Exclude system message

      // Remove oldest pairs if we exceed token limit or pair limit
      if ((estimated_tokens > max_batch_tokens ||
           pair_count >= max_context_pairs) &&
          chat_messages.size() > 3) {
        std::printf(
            "Context management: %d tokens, %d pairs - trimming history\n",
            estimated_tokens, pair_count);
      }

      while ((estimated_tokens > max_batch_tokens ||
              pair_count >= max_context_pairs) &&
             chat_messages.size() > 3) {
        // Keep system message (index 0), remove oldest user/assistant pair
        free((void*)chat_messages[1].content); // Free user message
        free((void*)chat_messages[2].content); // Free assistant message
        chat_messages.erase(chat_messages.begin() + 1,
                            chat_messages.begin() + 3);
        pair_count--;
        estimated_tokens = EstimateTokenCount(); // Recalculate
      }

      // Add current user message
      chat_messages.push_back({"user", strdup(user_message.c_str())});

      int new_len = llama_chat_apply_template(
          tmpl, chat_messages.data(), chat_messages.size(), true,
          formatted.data(), formatted.size());
      if (new_len > (int)formatted.size()) {
        formatted.resize(new_len);
        new_len = llama_chat_apply_template(tmpl, chat_messages.data(),
                                            chat_messages.size(), true,
                                            formatted.data(), formatted.size());
      }

      if (new_len <= 0) {
        std::printf(
            "Failed to apply the chat template. Resorting to manual prompt\n");
        // Fallback to manual prompt if chat template fails
        std::string fallback_prompt = "<start_of_turn>user\n" + user_message +
                                      "<end_of_turn>\n<start_of_turn>model\n";

        // Tokenize the fallback prompt
        const int n_tokens_check =
            -llama_tokenize(vocab, fallback_prompt.c_str(),
                            fallback_prompt.size(), nullptr, 0, true, true);
        if (n_tokens_check <= 0) {
          response.clear();
          return response;
        }

        prompt_tokens.resize(n_tokens_check);
        const int n_tokens = llama_tokenize(
            vocab, fallback_prompt.c_str(), fallback_prompt.length(),
            prompt_tokens.data(), prompt_tokens.size(), true, true);

        if (n_tokens < 0 || n_tokens != n_tokens_check) {
          response.clear();
          return response;
        }
      } else {
        // Use the formatted chat template
        std::string formatted_prompt(formatted.begin(),
                                     formatted.begin() + new_len);

        // Tokenize the formatted prompt
        const int n_tokens_check =
            -llama_tokenize(vocab, formatted_prompt.c_str(),
                            formatted_prompt.size(), nullptr, 0, true, true);
        if (n_tokens_check <= 0) {
          response.clear();
          return response;
        }

        prompt_tokens.resize(n_tokens_check);
        const int n_tokens = llama_tokenize(
            vocab, formatted_prompt.c_str(), formatted_prompt.length(),
            prompt_tokens.data(), prompt_tokens.size(), true, true);

        if (n_tokens < 0 || n_tokens != n_tokens_check) {
          response.clear();
          return response;
        }
      }

      // Check context usage and manage properly
      int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0);
      int n_ctx = llama_n_ctx(ctx);

      // Clear context if approaching limit
      if (n_ctx_used + prompt_tokens.size() > n_ctx * 0.8) {
        llama_memory_clear(llama_get_memory(ctx), false);
      }

      // Clear previous context and evaluate prompt
      llama_memory_t memory = llama_get_memory(ctx);
      llama_memory_clear(memory, false);

      // Safety check: ensure we don't exceed batch limit
      if (prompt_tokens.size() > (size_t)llama_n_batch(ctx)) {
        std::printf("Error: Prompt tokens (%zu) exceed batch limit (%d)\n",
                    prompt_tokens.size(), llama_n_batch(ctx));
        response.clear();
        return response;
      }

      llama_batch batch =
          llama_batch_get_one(prompt_tokens.data(), prompt_tokens.size());

      // Generate response tokens - optimized for real-time
      response.clear();

      const int max_tokens =
          1024; // Limit response length for real-time performance
      llama_token new_token_id;
      while (true) {
        int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(ctx), 0);
        if (n_ctx_used + batch.n_tokens > n_ctx) {
          break;
        }

        if (llama_decode(ctx, batch)) {
          std::printf("llama_decode failed, stopping generation\n");
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

        // Early termination for real-time performance
        // Stop if we hit natural sentence boundaries
        // if (token_len > 0 && (token_str[token_len - 1] == '.' ||
        //                       token_str[token_len - 1] == '!' ||
        //                       token_str[token_len - 1] == '?')) {
        //   // Check if we have a reasonable amount of content
        //   if (response.length() > 20) {
        //     break;
        //   }
        // }
      }

      // Clean up the response - remove common model artifacts
      if (response.find("<end_of_turn>") != std::string::npos) {
        response = response.substr(0, response.find("<end_of_turn>"));
      }

      // Remove thinking tags if they still appear
      size_t think_start = response.find("<think>");
      if (think_start != std::string::npos) {
        size_t think_end = response.find("</think>", think_start);
        if (think_end != std::string::npos) {
          response.erase(think_start, think_end - think_start + 8);
        }
      }

      // Remove trailing whitespace
      while (!response.empty() && std::isspace(response.back())) {
        response.pop_back();
      }

      // Add the assistant's response to conversation history for future context
      if (!response.empty()) {
        chat_messages.push_back({"assistant", strdup(response.c_str())});
      }

      return response;
    }

  private:
    // Estimate token count for current conversation
    int EstimateTokenCount() {
      if (chat_messages.empty())
        return 0;

      // Create a temporary formatted version to count tokens
      const char* tmpl = llama_model_chat_template(model, nullptr);
      if (!tmpl)
        return 0;

      // Try to apply template and get length
      int estimated_len = llama_chat_apply_template(
          tmpl, chat_messages.data(), chat_messages.size(), true, nullptr, 0);

      if (estimated_len <= 0)
        return 0;

      // Rough estimate: average 3-4 characters per token for mixed
      // Japanese/English
      return estimated_len / 3;
    }

  public:
    // Method to clear conversation history and start fresh
    void ClearConversationHistory() {
      // Clean up existing content (except system message if it exists)
      for (size_t i = 1; i < chat_messages.size(); i++) {
        free((void*)chat_messages[i].content);
      }

      // Keep only system message if conversation was initialized
      if (conversation_initialized && !chat_messages.empty()) {
        chat_messages.resize(1); // Keep only system message
      } else {
        chat_messages.clear();
        conversation_initialized = false;
      }
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