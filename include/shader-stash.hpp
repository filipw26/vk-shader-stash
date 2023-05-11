#ifndef VK_SHADER_STASH_SHADER_STASH_HPP
#define VK_SHADER_STASH_SHADER_STASH_HPP

#include "vulkan/vulkan_raii.hpp"
#include "shaderc/shaderc.hpp"

#include <iostream>
#include <string>
#include <filesystem>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include <condition_variable>
#include <chrono>
#include <utility>
#include <fstream>

namespace shs {

    using namespace std::chrono_literals;
    namespace fs = std::filesystem;

    class ShaderStash {
    public:
        ShaderStash(vk::raii::Device& device_, fs::path shaderFolderPath_): device(device_), shaderFolderPath(std::move(shaderFolderPath_)) {}

        std::shared_ptr<vk::raii::ShaderModule> get(const std::string& shaderFileName) {
            auto result = this->shaders.find(shaderFileName);

            if (result == this->shaders.end()) return nullptr;

            return result->second.second;
        }

        std::chrono::duration<int64_t> getScanInterval() { return this->scanInterval; }
        ShaderStash& setScanInterval(std::chrono::duration<int64_t> interval) {
            this->scanInterval = interval;
            return *this;
        }

        bool getHotReload() const { return this->shouldHotReload; }
        ShaderStash& setHotReload(bool val) {
            this->shouldHotReload = val;

            bool isRunning = this->hotReloadThread.get_id() != std::jthread::id();
            if (val == isRunning) return *this;

            if (this->shouldHotReload) {
                this->hotReloadThread = std::jthread([this](const std::stop_token& stopToken){ this->watchFiles(stopToken); });
            } else {
                this->hotReloadThread.request_stop();
                this->hotReloadThread.join();
            }

            return *this;
        }

    private:
        static std::unordered_map<fs::path, shaderc_shader_kind> knownExtensions;

        static shaderc_shader_kind inferShaderKindFromFileExtension(const fs::path& extension) {
            auto lookupKind = knownExtensions.find(extension);

            if (lookupKind != knownExtensions.end()) return lookupKind->second;
            else return shaderc_glsl_infer_from_source;
        }

        static std::string readFileContents(const fs::path& filePath) {
            std::ifstream file {filePath};
            std::stringstream contentBuffer;
            contentBuffer << file.rdbuf();

            return contentBuffer.str();
        }

        void watchFiles(const std::stop_token& stopToken) {
            std::chrono::time_point<std::chrono::file_clock> lastScanTimestamp;

            shaderc::Compiler shaderCompiler;

            std::mutex mutex;
            std::unique_lock<std::mutex> lock(mutex);

            while(!stopToken.stop_requested()) {
                fs::recursive_directory_iterator dir {shaderFolderPath};

                for (const fs::directory_entry& entry : dir) {
                    if (!entry.is_regular_file() || entry.last_write_time() < lastScanTimestamp || !entry.exists()) continue;

                    std::string filename = entry.path().filename().string();

                    std::string fileContents = readFileContents(entry.path());
                    std::size_t fileContentHash = std::hash<std::string>{}(fileContents);

                    auto stashEntry = this->shaders.find(filename);

                    bool isPresentInStash = stashEntry != this->shaders.end();
                    bool isContentTheSame = isPresentInStash && stashEntry->second.first == fileContentHash;
                    if (isPresentInStash && isContentTheSame) continue;

                    shaderc_shader_kind shaderKind = inferShaderKindFromFileExtension(entry.path().extension());

                    shaderc::SpvCompilationResult compilationResult = shaderCompiler.CompileGlslToSpv(
                            fileContents,
                            shaderKind,
                            reinterpret_cast<const char*>(filename.c_str()));

                    std::vector<uint32_t> spirvCode = {compilationResult.cbegin(), compilationResult.cend()};

                    vk::raii::ShaderModule shaderModule = this->device.createShaderModule({{}, spirvCode});

                    std::pair<std::size_t, std::shared_ptr<vk::raii::ShaderModule>> p =
                            {fileContentHash, std::make_shared<vk::raii::ShaderModule>(std::move(shaderModule))};

                    if (isPresentInStash) {
                        stashEntry->second = std::move(p);
                        std::cout << "Updated " << entry.path().relative_path() << " shader\n";
                    } else {
                        shaders.insert({std::move(filename), std::move(p)});
                        std::cout << "Added new " << entry.path().relative_path() << " shader\n";
                    }
                }

                lastScanTimestamp = std::chrono::file_clock::now();

                std::condition_variable_any().wait_for(mutex, stopToken, 1s, []{return false;});
            }
        }

        // Config
        std::chrono::duration<int64_t> scanInterval;


        vk::raii::Device& device;

        fs::path shaderFolderPath;
        bool shouldHotReload = false;
        std::jthread hotReloadThread;

        // Filename -> {Content Hash, ShaderModule}
        std::unordered_map<std::string, std::pair<std::size_t, std::shared_ptr<vk::raii::ShaderModule>>> shaders;
    };

    std::unordered_map<fs::path, shaderc_shader_kind> ShaderStash::knownExtensions = {
            {".vert", shaderc_vertex_shader}, {".frag", shaderc_fragment_shader},
            {".geom", shaderc_geometry_shader}, {".tesc", shaderc_tess_control_shader},
            {".tese", shaderc_tess_evaluation_shader}, {".comp", shaderc_compute_shader}
        };
}

#endif //VK_SHADER_STASH_SHADER_STASH_HPP
