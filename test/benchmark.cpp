#include "gtest/gtest.h"

#include <llmr/map/map.hpp>
#include <llmr/util/image.hpp>
#include <llmr/util/io.hpp>
#include <llmr/util/std.hpp>

#include <rapidjson/document.h>

#include "../common/headless_view.hpp"

#include <dirent.h>
#include <numeric>


const std::string base_directory = []{
    std::string fn = __FILE__;
    fn.erase(fn.find_last_of("/"));
    return fn + "/fixtures/styles";
}();

class Benchmark : public ::testing::TestWithParam<std::string> {};

TEST_P(Benchmark, Render) {
    using namespace llmr;

    const std::string &base = GetParam();

    const std::string style = util::read_file(base_directory + "/" + base + ".style.json");
    const std::string info = util::read_file(base_directory + "/" + base + ".info.json");

    // Parse settings.
    rapidjson::Document doc;
    doc.Parse<0>((const char *const)info.c_str());
    ASSERT_EQ(false, doc.HasParseError());
    ASSERT_EQ(true, doc.IsObject());

    // Setup OpenGL
    HeadlessView view;
    Map map(view);

    for (auto it = doc.MemberBegin(), end = doc.MemberEnd(); it != end; it++) {
        const std::string name { it->name.GetString(), it->name.GetStringLength() };
        const rapidjson::Value &value = it->value;
        ASSERT_EQ(true, value.IsObject());
        if (value.HasMember("center")) ASSERT_EQ(true, value["center"].IsArray());

        const std::string actual_image = base_directory + "/" + base + "/" + name +  ".bench.png";

        const double zoom = value.HasMember("zoom") ? value["zoom"].GetDouble() : 0;
        const double bearing = value.HasMember("bearing") ? value["bearing"].GetDouble() : 0;
        const double latitude = value.HasMember("center") ? value["center"][rapidjson::SizeType(0)].GetDouble() : 0;
        const double longitude = value.HasMember("center") ? value["center"][rapidjson::SizeType(1)].GetDouble() : 0;
        const unsigned int width = value.HasMember("width") ? value["width"].GetUint() : 512;
        const unsigned int height = value.HasMember("height") ? value["height"].GetUint() : 512;
        std::vector<std::string> classes;
        if (value.HasMember("classes")) {
            const rapidjson::Value &js_classes = value["classes"];
            ASSERT_EQ(true, js_classes.IsArray());
            for (rapidjson::SizeType i = 0; i < js_classes.Size(); i++) {
                const rapidjson::Value &js_class = js_classes[i];
                ASSERT_EQ(true, js_class.IsString());
                classes.push_back({ js_class.GetString(), js_class.GetStringLength() });
            }
        }

        map.setStyleJSON(style);
        map.setAppliedClasses(classes);

        view.resize(width, height);
        map.resize(width, height);
        map.setLonLatZoom(longitude, latitude, zoom);
        map.setAngle(bearing);

        // Run the loop. It will terminate when we don't have any further listeners.
        map.run();

        // Now that we have initialized all resources, we can start running the benchmark.
        const std::unique_ptr<uint32_t[]> pixels(new uint32_t[width * height]);

        struct BenchmarkIteration {
            size_t index;
            timestamp start;
            timestamp end;
            inline operator int64_t() const { return end - start; }
        };

        const size_t iterations = 10000;

        std::array<BenchmarkIteration, iterations> timings;

        // Do a hundred renders to warm up the cache.
        for (size_t i = 0; i < 100; i++) {
            map.render();
        }

        // Now start the real benchmark.
        for (size_t i = 0; i < iterations; i++) {
            timings[i].index = i;
            timings[i].start = util::now();

            map.render();

            glFinish();

            timings[i].end = util::now();
        }

        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.get());

        const std::string image = util::compress_png(width, height, pixels.get(), true);
        util::write_file(actual_image, image);

        const int64_t avg = std::accumulate(timings.begin(), timings.end(), int64_t(0)) / timings.size();
        const double variance = std::accumulate(timings.begin(), timings.end(), 0.0, [=](double total, const BenchmarkIteration &it) {
            const double diff = double(it - avg) / 1_microsecond;
            return total + diff * diff;
        }) / timings.size();
        const double stddev = std::sqrt(variance);
        const auto minmax = std::minmax_element(timings.begin(), timings.end());
        const int64_t min = *minmax.first;
        const int64_t max = *minmax.second;

        std::sort(timings.begin(), timings.end());
        const int64_t median = timings[timings.size() / 2];

        fprintf(stderr, "%s: iterations:%lu  min:% 9.3fµs  max:% 12.3fµs  avg:% 9.3fµs  median:% 9.3fµs  stddev:% 9.3f\n",
            name.c_str(), timings.size(),
            double(min) / 1_microsecond,
            double(max) / 1_microsecond,
            double(avg) / 1_microsecond,
            double(median) / 1_microsecond,
            stddev);

    }

}

INSTANTIATE_TEST_CASE_P(Benchmark, Benchmark, ::testing::Values(std::string("world-no-aa")));
