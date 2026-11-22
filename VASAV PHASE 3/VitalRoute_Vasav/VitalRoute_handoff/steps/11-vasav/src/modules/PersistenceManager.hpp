// PersistenceManager.hpp - plain-text records under data/records/.
//
//   patients.dat  P|id|name|age|phone|kind|severity|seq|admitted|status|
//                   complaint|department|bed code|bed label|doctor|discharged
//   staff.dat     D|id|name|age|phone|department|shift|specialty|fee|next free|seen
//                 N|id|name|age|phone|department|shift|ward
//                 A|id|name|age|phone|department|shift|office
//   visits.dat    V|patient|date|doctor|diagnosis|prescriptions
//   bills.dat     B|bill|patient|opened|settled|settled on|deduction|
//                   provider|policy|kind|value|cap
//                 C|bill|category|description|date|parameters...
//
// Fields are separated by '|'; '|', '\' and line breaks inside a field are
// escaped with '\'. Everything is written with ofstream and read back with
// ifstream; loading re-registers each patient, which rebuilds the hash and
// AVL indexes in memory.
#ifndef VR_PERSISTENCE_MANAGER_HPP
#define VR_PERSISTENCE_MANAGER_HPP

#include "BillingEngine.hpp"
#include "PatientRegistry.hpp"
#include "StaffRegistry.hpp"

#include <cstddef>
#include <string>
#include <vector>

class PersistenceManager {
public:
    struct Counts {
        std::size_t patients = 0;
        std::size_t staff = 0;
        std::size_t visits = 0;
        std::size_t bills = 0;
        std::size_t charges = 0;
    };

    explicit PersistenceManager(std::string directory = "data/records");

    bool hasSavedRecords() const;
    // Writes all four files (each via a temporary file, then renamed).
    // Throws std::runtime_error if a file cannot be written.
    Counts save(const PatientRegistry& patients, const StaffRegistry& staff,
                const BillingEngine& billing) const;
    // Clears the three stores and reloads them. Throws std::runtime_error
    // naming the file and line of the first bad record.
    Counts load(PatientRegistry& patients, StaffRegistry& staff, BillingEngine& billing) const;

    const std::string& directory() const { return dir_; }
    std::string pathOf(const std::string& file) const;

    static std::string escape(const std::string& field);
    static std::vector<std::string> split(const std::string& line);

private:
    std::string dir_;
};

#endif // VR_PERSISTENCE_MANAGER_HPP
