#pragma once

#include <memory>
#include <set>
#include <string>
#include <functional>
#include <vector>

#include "definitions.h"

namespace dsk_tools {

    #define VIEWER_OUTPUT_TEXT        0
    #define VIEWER_OUTPUT_PICTURE     1

    class Viewer {
    public:
        virtual ~Viewer() {}
        virtual std::string process_as_text(const BYTES & data, const std::string & cm_name) {return "";};
        virtual int get_output_type() const = 0;
        virtual std::string get_type() const = 0;
        virtual std::string get_subtype() const = 0;
    };

    class ViewerManager {
    public:
        using Creator = std::function<std::unique_ptr<Viewer>()>;

        static ViewerManager& instance() {
            static ViewerManager inst;
            return inst;
        }

        void register_viewer(const std::string& id_type, const std::string& id_subtype, Creator creator) {
            creators.emplace_back(id_type, id_subtype, creator);
        }

        std::unique_ptr<Viewer> create(const std::string& id_type, const std::string& id_subtype) const {
            for (const auto& entry : creators) {
                if (entry.id_type == id_type && entry.id_subtype == id_subtype) {
                    return (entry.creator)();
                }
            }
            return nullptr;
        }

        std::vector<std::string> list_types() const {
            std::set<std::string> unique_types;
            for (const auto& entry : creators) {
                unique_types.insert(entry.id_type);
            }
            return std::vector<std::string>(unique_types.begin(), unique_types.end());
        }

        std::vector<std::string> list_subtypes(const std::string& match_id_type) const {
            std::vector<std::string> results;
            for (const auto& entry : creators) {
                if (entry.id_type == match_id_type) {
                    results.emplace_back(entry.id_subtype);
                }
            }
            return results;
        }

    private:
        struct Entry {
            std::string id_type;
            std::string id_subtype;
            Creator creator;

            Entry(const std::string& t, const std::string& s, Creator c)
                : id_type(t), id_subtype(s), creator(c) {}
        };

        std::vector<Entry> creators;
    };

    template<typename T>
    class ViewerRegistrar {
    public:
        ViewerRegistrar() {
            T temp;
            ViewerManager::instance().register_viewer(temp.get_type(), temp.get_subtype(), []() -> std::unique_ptr<Viewer> {
                return std::unique_ptr<Viewer>(new T());
            });
        }
    };

}
