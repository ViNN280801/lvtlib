#ifndef LVT_HPP
#define LVT_HPP

#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <regex>
#include <span>
#include <tuple>
#include <concepts>
#include <numeric>
#include <iterator>
#include <iomanip>
#include <chrono>
#include <ratio>
#include <fstream>
#include <filesystem>
#include <random>
#include <ctime>
#include <utility>
#include <functional>
#include <source_location>
#include <format>
#include <cassert>

#include "lvt_impl.hpp"

#define ERRMSG(desc) std::cerr << std::format("ERROR: {}: {}({} line): {}: {}\n",          \
                                              std::chrono::system_clock::now(),            \
                                              std::source_location::current().file_name(), \
                                              std::source_location::current().line(),      \
                                              __PRETTY_FUNCTION__, desc);
#define LOGMSG(desc) std::clog << std::format("LOG: {}: {}({} line): {}: {}\n",            \
                                              std::chrono::system_clock::now(),            \
                                              std::source_location::current().file_name(), \
                                              std::source_location::current().line(),      \
                                              __PRETTY_FUNCTION__, desc);

using namespace std::chrono_literals;

namespace lvt
{
    /**
     * @brief Concept that checks if an element can be destroyed with std::destroy_at.
     * @tparam E Element type
     */
    template <typename E>
    concept default_erasable = requires(E *p) {
        std::destroy_at(p);
    };

    /**
     * @brief Concept that checks if an element can be destroyed using allocator traits.
     * @tparam E Element type
     * @tparam T Container type
     * @tparam A Allocator type
     */
    template <typename E, typename T, typename A>
    concept allocator_erasable = requires(A m, E *p) {
        requires std::same_as<typename T::allocator_type, typename std::allocator_traits<A>::rebind_alloc<E>>;
        std::allocator_traits<A>::destroy(m, p);
    };

    /**
     * @brief Concept that checks if a container is allocator-aware.
     * @tparam T Container type
     */
    template <typename T>
    concept allocator_aware = requires(T a) {
        {
            a.get_allocator()
        } -> std::same_as<typename T::allocator_type>;
    };

    /**
     * @brief Metafunction to identify if a type is a std::basic_string.
     * @tparam T Type to check
     */
    template <typename T>
    struct is_basic_string : std::false_type
    {
    };

    /**
     * @brief Metafunction to identify if a type is a std::basic_string (specialization for std::basic_string).
     * @tparam C Character type
     * @tparam T Traits type
     * @tparam A Allocator type
     */
    template <typename C, typename T, typename A>
    struct is_basic_string<std::basic_string<C, T, A>> : std::true_type
    {
    };

    /**
     * @brief Variable template to check if a type is a std::basic_string.
     * @tparam T Type to check
     */
    template <typename T>
    constexpr bool is_basic_string_v = is_basic_string<T>::value;

    /**
     * @brief Concept that checks if an element can be erased from a container.
     * @tparam E Element type
     * @tparam T Container type
     */
    template <typename E, typename T>
    concept erasable = (is_basic_string_v<T> && default_erasable<E>) || (allocator_aware<T> && allocator_erasable<E, T, typename T::allocator_type>) || (!allocator_aware<T> && default_erasable<E>);

    /**
     * @brief Concept that specifies the properties of a container type.
     * @tparam T Container type
     */
    template <typename T>
    concept Container = requires(T a, const T b) {
        // Requires the type to be regular and swappable
        requires std::regular<T>;
        requires std::swappable<T>;

        // Requires the element type to be erasable
        requires erasable<typename T::value_type, T>;

        // Requires specific type relationships for references and iterators
        requires std::same_as<typename T::reference, typename T::value_type &>;
        requires std::same_as<typename T::const_reference, const typename T::value_type &>;
        requires std::forward_iterator<typename T::iterator>;
        requires std::forward_iterator<typename T::const_iterator>;
        requires std::signed_integral<typename T::difference_type>;
        requires std::same_as<typename T::difference_type, typename std::iterator_traits<typename T::iterator>::difference_type>;
        requires std::same_as<typename T::difference_type, typename std::iterator_traits<typename T::const_iterator>::difference_type>;
        {
            a.begin()
        } -> std::same_as<typename T::iterator>;
        {
            a.end()
        } -> std::same_as<typename T::iterator>;
        {
            b.begin()
        } -> std::same_as<typename T::const_iterator>;
        {
            b.end()
        } -> std::same_as<typename T::const_iterator>;
        {
            a.cbegin()
        } -> std::same_as<typename T::const_iterator>;
        {
            a.cend()
        } -> std::same_as<typename T::const_iterator>;
        {
            a.size()
        } -> std::same_as<typename T::size_type>;
        {
            a.max_size()
        } -> std::same_as<typename T::size_type>;
        {
            a.empty()
        } -> std::convertible_to<bool>;

        // Add the std::ranges::range requirement
        requires std::ranges::range<T>;
    };

    /**
     * @brief Concept that specifies all types that can be convert to "std::string_view" type
     * For example, "char", "const char *", "std::string", etc.
     */
    template <typename T>
    concept StringConvertible = std::is_convertible_v<T, std::string_view>;

    /**
     * @brief Concept that checks available of all comparison operators
     * @tparam a first variable
     * @tparam b second variable
     */
    template <typename T>
    concept Comparable = requires(T a, T b) {
        {
            a < b
        } -> std::convertible_to<bool>;
        {
            a == b
        } -> std::convertible_to<bool>;
        {
            a != b
        } -> std::convertible_to<bool>;
        {
            a > b
        } -> std::convertible_to<bool>;
        {
            a <= b
        } -> std::convertible_to<bool>;
        {
            a >= b
        } -> std::convertible_to<bool>;
    };

    /**
     * @brief Concept that checks if variable has output operator
     * @tparam a variable to check
     * @param os output stream
     */
    template <typename T>
    concept Printable = requires(T a, std::ostream &os) {
        {
            os << a
        } -> std::same_as<std::ostream &>;
    };

    /**
     * @brief Describes matrix. Checks type 'R' representes a 2D-array of elements with type 'T'
     * by verifying that the elements within 'R' can be convertible to 'T'
     */
    template <typename R, typename T>
    concept Matrix = std::convertible_to<std::ranges::range_reference_t<std::ranges::range_reference_t<R>>, T>;

    namespace print
    {
        // Prints array to terminal
        template <typename T>
        extern void printVec(std::vector<T> const &);

        // Since C++20 (need std::span)
        // Prints range to terminal
        template <typename T>
        void print_range(std::span<const T>);

        // Prints range to terminal by iterators
        template <typename Iter>
        void print_range(Iter, Iter);

        // Prints matrix to terminal
        template <typename T>
        void printMatrix(std::vector<std::vector<T>> const &);

        /**
         * @brief Prints matrix
         * @tparam matrix matrix to print
         */
        template <Printable T>
        void printMatrix(Matrix<T> auto const &matrix);

        // Prints vector of pairs to terminal
        template <typename T1, typename T2>
        void print_pair_vec(const std::vector<std::pair<T1, T2>> &);

        // Prints dictionary to terminal (need std::map)
        template <typename T1, typename T2>
        void print_dictionary(const std::map<T1, T2> &);

        // Prints tuple to terminal (need std::tuple)
        template <typename TupleType, size_t TupleSize = std::tuple_size<TupleType>::value>
        void printTuple(const TupleType &);
    }

    namespace checkings
    {
        // Returns true if string is a unsigned integer number (unsigned short, unsigned int, size_t, etc.)
        bool is_uint_number(const std::string &);

        // Returns true if string is a signed integer number (short, int, long, etc.)
        bool is_int_number(const std::string &);

        // Returns true if string is a floating number (float, double, etc.)
        bool is_floating_number(const std::string &);

        // Returns true if both are equals
        template <typename T>
        bool is_equal(const T &, const T &);

        // Returns true if first lower than the second
        template <typename T>
        bool is_lower(const T &, const T &);

        // Returns true if first bigger than the second
        template <typename T>
        bool is_bigger(const T &, const T &);

        // Returns "true" if '__ch' is vowel, otherwise - "false"
        bool is_vowel(char);

        // Returns "true" if type of passed arg is arythmetic type of 'char'
        template <typename T>
        consteval bool isArithmeticType([[maybe_unused]] const T &) noexcept;

        /**
         * @brief Checker for bracket sequence that could include: ()[]{}
         * @param seq sequence of brackets
         * @return `true` if sequence is valid, otherwise - `false`
         */
        bool isBracketSequenceValid(std::string_view seq);
    }

    namespace convert
    {
        // Returns string parameter as T
        template <typename T>
        T str_to_T(const std::string &);

        // Returns 'T' as string
        template <typename T>
        std::string T_to_str(const T &);

        // Returns array that formed from the 2d array
        template <typename T>
        std::vector<T> matrixToArr(std::vector<std::vector<T>> const &);

        // Returns 2d array that formed from the 1d array
        template <typename T>
        std::vector<std::vector<T>> arrToMatrix(std::vector<T> const &, size_t, size_t);
    }

    namespace string
    {
        namespace modifying
        {
            // Turning all characters in string to lowercase
            std::string str_to_lower(const std::string &);

            // Makes all characters in string to uppercase
            std::string str_to_upper(const std::string &);

            // Returns string '__str' without vowels
            std::string remove_vowels(const std::string &);

            // Since C++20 (need std::erase_if())
            // Returns string '__str' without vowels
            std::string remove_vowels_cxx_20(const std::string &);

            // Returns string '__str' without vowels (need std::regex)
            std::string remove_vowels_regex(const std::string &);

            // Returns string '__str' without vowels
            void remove_vowels(std::string &__str);

            // Since C++20 (need std::erase_if())
            // Returns string '__str' without vowels
            void remove_vowels_cxx_20(std::string &);

            // Returns string '__str' without vowels (need std::regex)
            void remove_vowels_regex(std::string &);

            // Removing consecutive same characters from some range
            std::string remove_consecutive_spaces(const std::string &);

            // Removing consecutive same characters from some range
            void remove_consecutive_spaces(std::string &);
        }

        /**
         * @brief Generates string with specified multiple args
         * @tparam args arguments of type that can be convert to string
         * @return String composed from all arguments
         */
        template <typename... Args>
        constexpr std::string gen_str(Args &&...args);

        /**
         * @brief Finds the length of the longest substring without repeating characters.
         * This method employs a sliding window approach, continuously updating the window.
         * @param s The input string_view.
         * @return The length of the longest substring without repeating characters.
         */
        int lengthOfLongestSubstring(std::string_view s);
    }

    namespace input
    {
        // Returns user's input as unsigned integer number
        template <typename T>
        T input_to_uint(const char *msg = "");

        // Returns user's input as signed integer number
        template <typename T>
        T input_to_int(const char *msg = "");

        // Returns user's input as floating number
        template <typename T>
        T input_to_floating(const char *msg = "");
    }

    namespace random
    {
        // Returns one random double number
        double create_random_double(const double &__lower = 0.0, const double &__upper = 10.0);

        // Returns random string
        /* Hint: You must declare at the top following line:
            #define __GENERATE__ALL__SYMBOLS__ for generate string consisting of all symbols
            or
            #define __GENERATE__ONLY__DIGITS__ for generate string consisting of only digits */
        std::string generateRandomString(size_t);

        // Returns random vector filled with integer numbers
        // 'vecSize' - size of the vector
        // 'from' - lower number
        // 'to' - higher number to generate
        std::vector<int> generateRandomIntVector(size_t vecSize = 10UL, int from = -50, int to = 50);

        // Returns matrix of integers that is filled with random numbers
        // Gets rows as a first parameter and columns as a second
        // Third param - offset, fourth - range
        std::vector<std::vector<int>> generateRandomIntMatrix(size_t rows, size_t cols,
                                                              int offset = 1, int range = 100);

        /**
         * @brief Fills vector with integer values
         * @param v vector
         * @param from min value to generate
         * @param to max value to generate
         */
        void fillVector(std::vector<int> &v, int from = 0, int to = 100);
    }

    namespace algorithm
    {
        namespace sorting
        {
            // Sorting 1d array by bubble sorting algorithm
            template <typename T>
            constexpr void bubbleSortAscending(std::vector<T> &);

            // Sorting 2d array by bubble sorting algorithm
            template <typename T>
            constexpr void bubbleSort2DAscending(std::vector<std::vector<T>> &);

            // Sorting elems in vector. Best case - O(n). Middle and worst cases - O(n^2)
            template <typename T>
            constexpr void insertionSortAscending(std::vector<T> &);

            // Sorting 2d array by insertion sorting algorithm
            template <typename T>
            constexpr void insertionSort2DAscending(std::vector<std::vector<T>> &);

            // Sorting vector by selection algorithm
            template <typename T>
            constexpr void selectionSortAscending(std::vector<T> &);

            // Sorting 2d array by selection sorting algorithm
            template <typename T>
            constexpr void selectionSort2DAscending(std::vector<std::vector<T>> &);

            // Sorting array by Shell sorting algorithm
            template <typename T>
            constexpr void ShellSortAscending(std::vector<T> &);

            // Sorting 2d array by Shell sorting algorithm
            template <typename T>
            constexpr void ShellSort2DAscending(std::vector<std::vector<T>> &);

            // Auxiliary method for quick sort algortihm
            template <typename T>
            constexpr void qSortAscending(std::vector<T> &arr, size_t, size_t);

            // Sorting array by quick sorting (Hoare sort) algorithm
            template <typename T>
            constexpr void quickSortAscending(std::vector<T> &);

            // Sorting 2d array by quick sorting (Hoare sort) algorithm
            template <typename T>
            constexpr void quickSort2DAscending(std::vector<std::vector<T>> &);

            // Sorting 1d array by bubble sorting algorithm
            template <typename T>
            constexpr void bubbleSortDescending(std::vector<T> &);

            // Sorting 2d array by bubble sorting algorithm
            template <typename T>
            constexpr void bubbleSort2DDescending(std::vector<std::vector<T>> &);

            // Sorting elems in vector. Best case - O(n). Middle and worst cases - O(n^2)
            template <typename T>
            constexpr void insertionSortDescending(std::vector<T> &);

            // Sorting 2d array by insertion sorting algorithm
            template <typename T>
            constexpr void insertionSort2DDescending(std::vector<std::vector<T>> &);

            // Sorting vector by selection algorithm
            template <typename T>
            constexpr void selectionSortDescending(std::vector<T> &);

            // Sorting 2d array by selection sorting algorithm
            template <typename T>
            constexpr void selectionSort2DDescending(std::vector<std::vector<T>> &);

            // Sorting array by Shell sorting algorithm
            template <typename T>
            constexpr void ShellSortDescending(std::vector<T> &);

            // Sorting 2d array by Shell sorting algorithm
            template <typename T>
            constexpr void ShellSort2DDescending(std::vector<std::vector<T>> &);

            // Auxiliary method for quick sort algortihm
            template <typename T>
            constexpr void qSortDescending(std::vector<T> &arr, size_t, size_t);

            // Sorting array by quick sorting (Hoare sort) algorithm
            template <typename T>
            constexpr void quickSortDescending(std::vector<T> &);

            // Sorting 2d array by quick sorting (Hoare sort) algorithm
            template <typename T>
            constexpr void quickSort2DDescending(std::vector<std::vector<T>> &);

            /**
             * @brief Recursive func helper for merge sort
             * @tparam arr array of elements -> arr[start;end)
             * @tparam result result vector
             * @param start left index
             * @param end right index
             */
            template <Comparable T>
            constexpr void mergeSortAscendingHelper(std::vector<T> &arr, std::vector<T> &result, size_t start, size_t end);

            /**
             * @brief Merge sort function. Best, middle and worth cases: 0(n*log(n))
             * @tparam vec array of elements
             */
            template <Comparable T>
            constexpr void mergeSortAscending(std::vector<T> &vec);

            /**
             * @brief Recursive func helper for merge sort
             * @tparam arr array of elements -> arr[start;end)
             * @tparam result result vector
             * @param start left index
             * @param end right index
             */
            template <Comparable T>
            constexpr void mergeSortHelper(std::vector<T> &arr, std::vector<T> &result, size_t start, size_t end);

            /**
             * @brief Merge sort function. Best, middle and worth cases: 0(n*log(n))
             * @tparam vec array of elements
             */
            template <Comparable T>
            constexpr void mergeSortDescending(std::vector<T> &vec);
        }

        // Returns array of digits in descending order
        // from integer and non-negative type of number
        template <typename unumeric_t>
        std::vector<int> split_number_on_digits(const unumeric_t &);

        // Returns number by vector of digits
        template <typename T = int, typename retType = T>
        retType composeNumberWithDigits(const std::vector<T> &vec);

        // Returns result of mathematical operation 'op' that applies to each element of range
        template <typename Iter, typename StartValue, typename Operation>
        auto accumulateData(Iter, Iter, StartValue, Operation);

        // Removing consecutive same characters from some range
        template <typename T>
        void remove_same_elems(std::vector<T> &);

        // Returns vector of pairs from two vectors same length
        template <typename T1, typename T2>
        std::vector<std::pair<T1, T2>> make_vector_of_pairs_by_two_vectors(const std::vector<T1> &,
                                                                           const std::vector<T2> &);

        // Returns vector of words in a string, except any spaces
        std::vector<std::string> split_str(const std::string &, const std::string & = " ");

        // Composing vector of string to a single string
        std::string vec_to_str(const std::vector<std::string> &);

        // Since C++20 (need std::span)
        // Composing vector of string to a single string
        std::string vec_to_str(std::span<const std::string>);

        // Returns iterator on output sequence that is one-past-last element
        // stored in the output sequence, which is contains all elements with their positions,
        // matching a predicate in a given range
        // 'first' - start iterator of the input sequence
        // 'last' - end iterator of the input sequence
        // 'dest' - start iterator of the output sequence
        template <typename InputIter, typename OutputIter, typename Predicate>
        OutputIter find_all(InputIter, InputIter,
                            OutputIter, Predicate);

        // Since C++ 20 (need std::span and <concepts>)
        // Returns sliced vector from 'first' to 'last': [first, last]
        template <typename T>
        std::vector<T> sliceVector(std::span<const T>, const std::integral auto &, const std::integral auto &);

        // Since C++ 20 (need <concepts>)
        // Slicing vector from 'first' to 'last': [first, last]
        template <typename T>
        void sliceVector(std::vector<T> &, const std::integral auto &first, const std::integral auto &);

        // Returns max subarray sum of contiguous elements in sequence
        // Solves the "Maximum subarray problem" with the "Kadane's Algorithm"
        template <typename T>
        T maxSubarraySum(std::span<const T>);

        // Returns vector of symbols in the string 'str', except any 'delim'eters
        std::vector<std::string> splitVecStringBy(std::string &, char) noexcept;

        // Returns count of unique symbols in the string 'str'
        int countOfUniqueSymbols(const std::string &);

        // Returns sum of only digits in the string 'str'
        int sumOfOnlyDigits(std::string);

        // Returns count of the first consecutive occurrences in the string
        constexpr int firstCountOfConsecutiveOccurrences(const std::string &);

        // Returns count of the first consecutive occurrences in the string
        // 'n'- is number of occurrence
        constexpr int countOfConsecutiveOccurrencesAt_n(const std::string &, size_t n = 1UL);

        // Returns maximum count of the consecutive occurrences in the string 's'
        constexpr int maxCountOfConsecutiveOccurrences(const std::string &);

        // Return vector of pairs that will store the only unique keys as a first value of pair
        // and sum of values of repeating keys as a second value of pair
        std::vector<std::pair<char, int>> compressTheVectorOfPairs(std::vector<std::string> const &);

        // Returns vector of string with all occurences
        // If you want to fill vector with all symbols which are don't match -> 'isMatch' should be -1
        std::vector<std::string> regexFindAll(std::string const &,
                                              std::string const &, int isMatch = 0);

        /// @brief Removes all punctuation signs from the string
        /// @param str string to remove punctuation
        void removePunct(std::string &str);

        /// @brief Searches occurrence: 's' == any of 'v'
        /// @param v vector of strings to search
        /// @param s string to search in vector of strings
        /// @return "true" if vector of strings contains specified string
        bool isContains(const std::vector<std::string> &v, const std::string &s);

        /// @brief Finding in the text all the words that occur in the same contexts (between the same words)
        /// @param text whole text (or any string)
        /// @return Vector of words which are occuring in the same contexts
        std::vector<std::string> getWordsInSameContexts(const std::string &text);

        /**
         * @brief Finds the most frequently element in the range
         * @tparam range range to search the most frequently element in there
         * @return The most frequently element in the range
         */
        template <typename T>
        constexpr T mostFreqElem(std::span<T const> range);

        /**
         * @brief Calculates 'k' most frequency elements in a range
         * @tparam range sequence of the elements
         * @param k count of most frequency elements to search
         * @return Vector of the most frequency elements with size 'k'
         */
        template <typename T, std::integral U>
        extern constexpr std::vector<T> kMostFreqElem(std::span<T const> range, U k);

        /**
         * @brief Searches for closest element to specified in a sequence
         * @tparam range sequence in which search
         * @tparam value value to find closest for it
         * @return Closest element to specified value
         */
        template <std::integral T>
        constexpr T findClosest(std::span<T const> range, T value);

        /**
         * @brief Calculates a common length of the all specified intervals
         * @param intervals array of intervals
         * @return Common length of the all intervals. -1 if smth wrong
         */
        int calculateIntervalsLength(std::vector<std::pair<int, int>> intervals);

        /**
         * @brief Calculates max product of 2 elems in sequence (signs are not taken in account).
         * @tparam range sequence of elements.
         * @return Max product of 2 elems. Returns min value of "unsigned long long" type if smth was wrong.
         */
        template <std::unsigned_integral T>
        unsigned long long maxPairwiseProduct(std::span<T const> range);

        /**
         * @brief Calculates max product of 3 elems in sequence (signs are taken in account).
         * @tparam range sequence of elements.
         * @return Max product of 3 elems. Returns min value of "long long" type if smth was wrong.
         */
        template <std::integral T>
        long long maxProductOf3Elems(std::span<T const> range);

        /**
         * @brief Generates new array that contains only unique elems
         * @tparam rangeA 1st range of elems
         * @tparam rangeB 2nd range of elems
         * @return Vector of an unique elems got from 2 sequences
         */
        template <std::integral T>
        constexpr std::vector<T> getUniqueElementsFromTwoSequences(std::span<T const> rangeA, std::span<T const> rangeB);

        /**
         * @brief Gathering all permutations to vector
         * @param s string
         * @return Vector that contains all permutations of string 's'
         */
        std::vector<std::string> stringPermutations(std::string s);

        /**
         * @brief Calculates 'n' elements from Tribonacci sequence by passes sequence
         * @param signature sequence of integer elements
         * @param n count of elements in the result sequence
         * @return Tribonacci sequence of 'n' size started with 'signature' elements
         */
        std::vector<int> tribonacci(std::vector<int> signature, int n);

        /**
         * @brief Calculates sum of two polynomials, assumes that coefficients in both args (polynomials)
         * are in correct order (for example, A(x) = 5x^3 + 2x^2 - 7x + 3) -> where 5 is the last index, 3 is the first.
         * @tparam a - first polynomial.
         * @tparam b - second polynomial.
         * @return C(x) - resulting polynomial in correct order (see above).
         */
        template <std::integral T>
        constexpr std::vector<T> sumOfPolynomials(std::span<T const> a, std::span<T const> b);

        /**
         * @brief Calculates sum of the matrices.
         * @tparam a - 1st matrix.
         * @tparam b - 2nd matrix.
         * @return Matrix where all elements are summarized (a + b).
         */
        template <std::integral T>
        std::vector<std::vector<T>> sumOfTheMatrices(Matrix<T> auto const &a, Matrix<T> auto const &b);

        /**
         * @brief Transposes matrix
         * @tparam T restictions: constructible, copy/move-assignable
         * @tparam matrix matrix to transpose
         * @return Transposed matrix as vector of vectors
         */
        template <typename T>
        std::vector<std::vector<T>> transposeMatrix(Matrix<T> auto const &matrix);

        /**
         * @brief Searches common prefix in sequence of strings.
         * @tparam strings sequence of strings
         * @return Common prefix in all passed strings
         */
        template <StringConvertible T>
        std::string commonPrefix(std::span<T const> strings);

        /**
         * @brief Finds common letters in a sequence of words
         * @param words sequence of words
         * @return Common letters as a word ("std::string" instance)
         */
        std::string commonLetters(std::vector<std::string> const &words);

        /**
         * @brief Joins passed strings with passed delimiter.
         * @param tokens strings to be combined.
         * @param delim delimiter by which they would joining.
         * @return String composed from passed range of strings with specified delimiter.
         */
        std::string join(std::span<std::string_view> tokens, std::string_view delim);

        /**
         * @brief Gets all non-empty dirs and subdirs
         * @param dirs path to directories
         * @return Set of non-empty directories in lexicographical order
         *
         * Example of usage:
         * std::vector<std::string> dirs;
         * std::string path;
         * while (std::getline(std::cin, path))
         *     dirs.emplace_back(path);
         * for (std::string_view dir : extractNonEmptyDirs(dirs))
         *     std::cout << dir << '\n';
         */
        std::vector<std::string> extractNonEmptyDirs(std::vector<std::string> dirs);

        /**
         * @brief Calculate the frequencies of symbolic n-grams in a text
         * @param words list of words
         * @param lengthOfNGramm length of each n-gram
         * @return Vector of words and their frequencies correspondingly.
         * Vector sorted by frequencies if freqs are equal - lexicograpically sorted
         */
        std::vector<std::pair<std::string, int>> calculateNGramFrequencies(std::vector<std::string> const &words,
                                                                           size_t lengthOfNGramm);

        /**
         * @brief Duplicates vector elements to itself
         * @tparam v vector to duplicate
         */
        template <typename T>
        void duplicateVector(std::vector<T> &v);

        /**
         * @brief Approximate Binary Search between two vectors.
         * This function performs an approximate binary search between two sorted vectors.
         * It finds the closest value in `vec1` for each element in `vec2`.
         * If there are multiple elements in `vec1` with the same absolute difference to an
         * element in `vec2`, the lower value is chosen.
         * @tparam T The type of elements in the input vectors.
         * @param vec1 First sorted vector
         * @param vec2 Second vector for which approximate search is performed
         * @return A vector containing the closest values from `vec1` for each element in `vec2`
         */
        template <typename T>
        std::vector<T> ApproxBinSearch(std::span<T const> vec1, std::span<T const> vec2);

        /**
         * @brief Comparator for the map
         * @tparam Map template parameter that defines map
         * @param lhs first map
         * @param rhs second map
         * @return `true` if sizes and contents of the both maps are equal, otherwise `false`.
         */
        template <typename Map>
        constexpr bool map_compare(Map const &lhs, Map const &rhs);
    }

    namespace big_numbers
    {
        // Calculates sum of 2 big numbers represented as array of integer
        void sum(const std::vector<int> &, const std::vector<int> &, std::vector<int> &);

        // Calculates product of 2 big numbers represented as array of integer
        void product(const std::vector<int> &, const std::vector<int> &, std::vector<int> &);

        // Returns factorial of number
        std::string factorial(int);
    }

    namespace time
    {
        /**
         * @brief A high-resolution timer for measuring elapsed time.
         * The Timer class provides functionality to measure the elapsed time between
         * starting and stopping the timer. It uses the high-resolution clock for precision.
         */
        class Timer
        {
        private:
            std::chrono::high_resolution_clock::time_point m_start_tp, m_end_tp;
            bool m_isstarted{false};

        public:
            /// @brief Defaulted ctor
            explicit Timer() = default;

            /**
             * @brief Starts the timer.
             * Marks the starting point of the timer by recording the current high-resolution time.
             */
            void startTimer()
            {
                m_isstarted = true;
                m_start_tp = std::chrono::high_resolution_clock::now();
            }

            /**
             * @brief Stops the timer.
             * Marks the ending point of the timer by recording the current high-resolution time.
             */
            void stopTimer()
            {
                m_isstarted = false;
                m_end_tp = std::chrono::high_resolution_clock::now();
            }

            /**
             * @brief Gets the elapsed time in milliseconds.
             * Calculates and returns the elapsed time in milliseconds between the start and stop points.
             * It is necessary to stop the timer before calling this function.
             * @return Elapsed time in milliseconds
             */
            unsigned elaplsedTimeMS() const
            {
                assert(!m_isstarted);
                auto elapsed{m_end_tp - m_start_tp};
                auto ms{std::chrono::duration_cast<std::chrono::milliseconds>(elapsed)};
                return ms.count();
            }
        };

        /**
         * @brief Measures the execution time of a callable function with the provided arguments.
         * This function executes the given callable object and measures the elapsed time
         * between the start and stop points using a Timer. The result is the elapsed time
         * in milliseconds.
         * @tparam Callable Type of the callable object (function, lambda, etc.)
         * @tparam Args Parameter types for the callable object
         * @param callable The callable object to be measured
         * @param args Arguments to be forwarded to the callable object
         * @return Elapsed time in milliseconds
         */
        template <typename Callable, typename... Args>
        unsigned int measureExecutionTime(Callable &&callable, Args &&...args)
        {
            static_assert(std::is_invocable_v<Callable, Args...>, "Callable must be invocable with Args");

            Timer timer;
            timer.startTimer();
            std::invoke(std::forward<Callable>(callable), std::forward<Args>(args)...);
            timer.stopTimer();
            return timer.elaplsedTimeMS();
        }

        // Printing current time to terminal at specified format
        void printCurTime(const StringConvertible auto &);

        // Printing duration of executing some code snippet
        // At first, where do you want to start the timer, you need to write following line:
        // auto start{std::chrono::high_resolution_clock::now()};
        // and then execute this func
        void printExecutionTime(const auto &);

        /**
         * @brief Converts string to a formatted time (check 'time_t_to_str()' function)
         * @param str string to convert to the time format
         * @param format format in which will be time represented
         * @return "time_t" time
         */
        std::time_t str_to_time_t(std::string const &str, const char *format);

        /**
         * @brief Converts "time_t" variable to a string (check 'str_to_time_t()' function)
         * @param time time that will be converted
         * @param format format in which will be time represented
         * @return Converted time
         */
        std::string time_t_to_str(std::time_t const &time, char const *format);

        /// @brief Generates random date of birth with age in interval [18; 100] (by default)
        /// @param lowestAge lowest age of a person - by default is 18
        /// @param highestAge highest age of a person - by default is 100
        /// @return Date of birth represented as a string with format: "dd/mm/yyyy"
        std::string generateRandomDateOfBirth(int lowestAge = 18, int highestAge = 100);
    }

    namespace files
    {
        // Returns all content from a file as a "std::string"
        std::string readFileToStr(std::string const &);

        // Returns size of the file as a "std::size_t"
        size_t getSizeOfTheFile(std::string const &);

        /// @brief Checks if file with passed filename exists
        /// @param filename name of file to do existance check
        /// @return "true" if file exists, otherwise - "false"
        bool exists(std::string const &filename);

        /**
         * @brief Collect all filenames with specified mask into a vector
         * @param path path where to search
         * @param mask file mask
         * @return Filenames with specified mask in the vector
         */
        std::vector<std::string> getFilenamesByMask(
            std::filesystem::path const &path = std::filesystem::current_path(),
            std::string const &mask = ".*\\.txt$");

        /**
         * @brief Collect all filenames with specified mask into a vector
         * @param path path where to search
         * @param mask file mask
         * @return Filenames with specified mask in the vector
         */
        std::vector<std::string> getFilenamesByMaskInDirsAndSubdirs(
            std::filesystem::path const &path = std::filesystem::current_path(),
            std::string const &mask = ".*\\.txt$");
    }
}

#include "lvt_impl.hpp"

#endif // LVT_HPP
