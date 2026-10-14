#include <zint.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

// Zint encodes; ZXing never participates in fixture creation. PGM avoids
// an extra image encoder dependency and preserves exact raster bytes.
int main(int argc, char** argv)
try {
    if (argc != 3) throw std::runtime_error("expected codes.tsv and output directory");
    if (ZBarcode_Version() != 21500) throw std::runtime_error("unexpected Zint version");
    std::ifstream definitions(argv[1]);
    if (!definitions) throw std::runtime_error("cannot read fixture definitions");
    std::filesystem::create_directories(argv[2]);
    std::string line;
    int count = 0;
    while (std::getline(definitions, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream fields(line);
        std::string id, symbology, format, payload;
        if (!std::getline(fields, id, '\t') || !std::getline(fields, symbology, '\t') ||
            !std::getline(fields, format, '\t') || !std::getline(fields, payload))
            throw std::runtime_error("invalid fixture row");
        std::unique_ptr<zint_symbol, decltype(&ZBarcode_Delete)> symbol(ZBarcode_Create(), ZBarcode_Delete);
        if (!symbol) throw std::runtime_error("Zint allocation failed");
        symbol->symbology = std::stoi(symbology);
        if(id.find("unicode")!=std::string::npos) {
            symbol->input_mode=UNICODE_MODE;
            symbol->eci=26; // Explicit UTF-8 ECI, independent of reader charset guessing.
        }
        symbol->scale = 3;
        symbol->height = 60;
        symbol->whitespace_width = 12;
        symbol->whitespace_height = 12;
        symbol->show_hrt = 0;
        const int error = ZBarcode_Encode_and_Buffer(symbol.get(),
            reinterpret_cast<const unsigned char*>(payload.data()), static_cast<int>(payload.size()), 0);
        if (error != 0) throw std::runtime_error(id + ": " + symbol->errtxt);
        const auto path = std::filesystem::path(argv[2]) / (id + ".pgm");
        std::ofstream image(path, std::ios::binary);
        image << "P5\n" << symbol->bitmap_width << ' ' << symbol->bitmap_height << "\n255\n";
        for (int i = 0; i < symbol->bitmap_width * symbol->bitmap_height; ++i)
            image.put(static_cast<char>(symbol->bitmap[3 * i]));
        if (!image) throw std::runtime_error("fixture write failed");
        ++count;
    }
    if (count != 8) throw std::runtime_error("expected eight fixture definitions");
    std::cout << "Generated " << count << " independent Zint 2.15.0 fixtures\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
