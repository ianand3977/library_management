#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <curl/curl.h>
#include "json.hpp"  // Make sure this file is in the same directory

using json = nlohmann::json;

struct Book {
    int id;
    std::string title;
    std::string author;
    bool isBorrowed;

    Book() : id(0), isBorrowed(false) {}

    Book(int id, const std::string& title, const std::string& author)
        : id(id), title(title), author(author), isBorrowed(false) {}
};

class Library {
private:
    std::vector<Book> books;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
        size_t newLength = size * nmemb;
        try {
            s->append((char*)contents, newLength);
        } catch (std::bad_alloc& e) {
            return 0;
        }
        return newLength;
    }

    json fetchBookData(const std::string& query) {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;
        std::string url = "https://www.googleapis.com/books/v1/volumes?q=" + query;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK) {
                return json::parse(readBuffer);
            }
        }
        return nullptr;
    }

    void saveBooks() {
        std::ofstream file("books.txt", std::ios::out);
        for (const auto& book : books) {
            file << book.id << "\n"
                 << book.title << "\n"
                 << book.author << "\n"
                 << book.isBorrowed << "\n";
        }
        file.close();
    }

    void loadBooks() {
        std::ifstream file("books.txt", std::ios::in);
        if (file.is_open()) {
            Book book;
            while (file >> book.id) {
                file.ignore(); // Ignore newline
                std::getline(file, book.title);
                std::getline(file, book.author);
                file >> book.isBorrowed;
                books.push_back(book);
            }
            file.close();
        }
    }

public:
    Library() {
        loadBooks();
    }

    ~Library() {
        saveBooks();
    }

    void addBookFromAPI(const std::string& query) {
        json bookData = fetchBookData(query);
        if (!bookData.is_null()) {
            for (const auto& item : bookData["items"]) {
                int id = books.empty() ? 1 : books.back().id + 1;
                std::string title = item["volumeInfo"]["title"].get<std::string>();
                std::string author = item["volumeInfo"]["authors"][0].get<std::string>();
                books.push_back(Book(id, title, author));
            }
            saveBooks();
        } else {
            std::cout << "No books found.\n";
        }
    }

    void removeBook(int bookId) {
        books.erase(std::remove_if(books.begin(), books.end(),
                                   [bookId](const Book& book) { return book.id == bookId; }),
                    books.end());
        saveBooks();
    }

    void borrowBook(int bookId) {
        for (auto& book : books) {
            if (book.id == bookId && !book.isBorrowed) {
                book.isBorrowed = true;
                saveBooks();
                return;
            }
        }
        std::cout << "Book not available for borrowing.\n";
    }

    void returnBook(int bookId) {
        for (auto& book : books) {
            if (book.id == bookId && book.isBorrowed) {
                book.isBorrowed = false;
                saveBooks();
                return;
            }
        }
        std::cout << "Book not found or not borrowed.\n";
    }

    void displayBooks() const {
        for (const auto& book : books) {
            std::cout << "ID: " << book.id
                      << ", Title: " << book.title
                      << ", Author: " << book.author
                      << ", Borrowed: " << (book.isBorrowed ? "Yes" : "No") << "\n";
        }
    }

    int countBooks() const {
        return books.size();
    }
};

void displayMenu() {
    std::cout << "Library Management System\n";
    std::cout << "1. Add Book from API\n";
    std::cout << "2. Remove Book\n";
    std::cout << "3. Borrow Book\n";
    std::cout << "4. Return Book\n";
    std::cout << "5. Display Books\n";
    std::cout << "6. Count Books\n";
    std::cout << "7. Exit\n";
}

int main() {
    Library library;
    int choice;

    do {
        displayMenu();
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1: {
                std::string query;
                std::cout << "Enter book title or author to search: ";
                std::cin.ignore();
                std::getline(std::cin, query);
                library.addBookFromAPI(query);
                break;
            }
            case 2: {
                int id;
                std::cout << "Enter book ID to remove: ";
                std::cin >> id;
                library.removeBook(id);
                break;
            }
            case 3: {
                int id;
                std::cout << "Enter book ID to borrow: ";
                std::cin >> id;
                library.borrowBook(id);
                break;
            }
            case 4: {
                int id;
                std::cout << "Enter book ID to return: ";
                std::cin >> id;
                library.returnBook(id);
                break;
            }
            case 5:
                library.displayBooks();
                break;
            case 6:
                std::cout << "Total books: " << library.countBooks() << "\n";
                break;
            case 7:
                std::cout << "Exiting...\n";
                break;
            default:
                std::cout << "Invalid choice. Try again.\n";
        }
    } while (choice != 7);

    return 0;
}
