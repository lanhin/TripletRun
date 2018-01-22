// ==================
// @2018-01 by lanhin
// ACSA, USTC
// lanhin1@gmail.com
// ==================

/** Thanks to Doc. King's wheels.h @ACSA.
 */

#ifndef UTILS_H
#define UTILS_H

#define RESET       "\033[0m"
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"
#define BOLD_BLACK   "\033[1m\033[30m"
#define BOLD_RED     "\033[1m\033[31m"
#define BOLD_GREEN   "\033[1m\033[32m"
#define BOLD_YELLOW  "\033[1m\033[33m"
#define BOLD_BLUE    "\033[1m\033[34m"
#define BOLD_MAGENTA "\033[1m\033[35m"
#define BOLD_CYAN    "\033[1m\033[36m"
#define BOLD_WHITE   "\033[1m\033[37m"

#define log_error(x) do{std::cerr << RED << "[Error] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)
#define log_warning(x) do{std::cerr << YELLOW << "[Warning] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)
#define log_start(x) do{std::cout << GREEN << "[Start] " << RESET << x << std::endl;}while(false)
#define log_end(x) do{std::cout<< GREEN << "[OK] " << RESET << x << std::endl<<std::endl;}while(false)

#ifdef DEV_MODE
#define VAL(x) do{std::cout << #x << " = " << (x) << std::endl;}while(false)
#define VAL_ARRAY(a) do{     std::cout << #a << " = [";      std::for_each(a, a + DIM(a), [](int val__){std::cout << " " << val__ << " ";});      std::cout << "]\n";  } while(false)
#define log_func() do{std::cerr << CYAN << "[Function] " << __PRETTY_FUNCTION__ << RESET << std::endl;}while(false)
#define log_info(x) do{std::cerr << CYAN << "[Info] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)
#define log_ok(x) do{std::cerr << GREEN << "[Ok] " << x << " @" << __FILE__ << ":" << __LINE__ << RESET << std::endl;}while(false)
#else
#define VAL(x)
#define VAL_ARRAY(a)
#define log_func()
#define log_info(x)
#define log_ok(x)
#endif // DEV_MODE

#endif //UTILS_H
