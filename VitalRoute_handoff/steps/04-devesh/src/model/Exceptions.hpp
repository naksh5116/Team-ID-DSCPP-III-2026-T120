// Exceptions.hpp - VitalRoute's domain exceptions.
//
// Each derives directly from std::runtime_error, so callers can catch the
// specific failure or fall back to std::exception and print what().
#ifndef VR_EXCEPTIONS_HPP
#define VR_EXCEPTIONS_HPP

#include <stdexcept>
#include <string>

// No free bed of the clinically required type.
class BedUnavailableException : public std::runtime_error {
public:
    explicit BedUnavailableException(const std::string& message)
        : std::runtime_error(message) {}
};

// No patient record with the requested identifier.
class PatientNotFoundException : public std::runtime_error {
public:
    explicit PatientNotFoundException(const std::string& patientId)
        : std::runtime_error("no patient with ID " + patientId), id_(patientId) {}
    const std::string& patientId() const { return id_; }

private:
    std::string id_;
};

// Severity outside the Emergency Severity Index range 1-5.
class InvalidSeverityException : public std::runtime_error {
public:
    explicit InvalidSeverityException(int severity)
        : std::runtime_error("severity " + std::to_string(severity) +
                             " is outside the ESI range 1 (most critical) to 5"),
          severity_(severity) {}
    int severity() const { return severity_; }

private:
    int severity_;
};

// The floor plan has no path between two points.
class NoRouteFoundException : public std::runtime_error {
public:
    explicit NoRouteFoundException(const std::string& message)
        : std::runtime_error(message) {}
};

// An operation theatre slot cannot be booked or released.
class SlotUnavailableException : public std::runtime_error {
public:
    explicit SlotUnavailableException(const std::string& message)
        : std::runtime_error(message) {}
};

#endif // VR_EXCEPTIONS_HPP
