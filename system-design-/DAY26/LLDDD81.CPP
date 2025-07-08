#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

using namespace std;

// --- Product Class ---
class Product {
private:
    int sku;
    string name;
    double price;

public:
    Product(int id, string nm, double pr) : sku(id), name(nm), price(pr) {}

    int getSku() { return sku; }
    string getName() { return name; }
    double getPrice() { return price; }
};

// --- Product Factory ---
class ProductFactory {
public:
    static Product* createProduct(int sku) {
        string name;
        double price;

        if (sku == 101) { name = "Apple"; price = 20; }
        else if (sku == 102) { name = "Banana"; price = 10; }
        else if (sku == 103) { name = "Chocolate"; price = 50; }
        else { name = "Item" + to_string(sku); price = 100; }

        return new Product(sku, name, price);
    }
};

// --- InventoryStore Interface ---
class InventoryStore {
public:
    virtual ~InventoryStore() {}
    virtual void addProduct(Product* prod, int qty) = 0;
    virtual void removeProduct(int sku, int qty) = 0;
    virtual int checkStock(int sku) = 0;
    virtual vector<Product*> listAvailableProducts() = 0;
};

// --- DbInventoryStore Implementation ---
class DbInventoryStore : public InventoryStore {
private:
    map<int, int> stock;
    map<int, Product*> products;

public:
    ~DbInventoryStore() {
        for (auto& it : products) delete it.second;
    }

    void addProduct(Product* prod, int qty) override {
        int sku = prod->getSku();
        if (products.count(sku) == 0) {
            products[sku] = prod;
        } else {
            delete prod;
        }
        stock[sku] += qty;
    }

    void removeProduct(int sku, int qty) override {
        if (!stock.count(sku)) return;
        stock[sku] = max(0, stock[sku] - qty);
    }

    int checkStock(int sku) override {
        return stock.count(sku) ? stock[sku] : 0;
    }

    vector<Product*> listAvailableProducts() override {
        vector<Product*> available;
        for (auto& pair : stock) {
            if (pair.second > 0) available.push_back(products[pair.first]);
        }
        return available;
    }
};

// --- InventoryManager ---
class InventoryManager {
private:
    InventoryStore* store;

public:
    InventoryManager(InventoryStore* store) : store(store) {}

    void addStock(int sku, int qty) {
        Product* prod = ProductFactory::createProduct(sku);
        store->addProduct(prod, qty);
    }

    void removeStock(int sku, int qty) { store->removeProduct(sku, qty); }

    int checkStock(int sku) { return store->checkStock(sku); }

    vector<Product*> getAvailableProducts() { return store->listAvailableProducts(); }
};

// --- Cart ---
class Cart {
private:
    vector<pair<Product*, int>> items;

public:
    void addItem(int sku, int qty) {
        Product* p = ProductFactory::createProduct(sku);
        items.push_back({p, qty});
    }

    vector<pair<Product*, int>> getItems() { return items; }

    double getTotal() {
        double total = 0;
        for (auto& it : items)
            total += it.first->getPrice() * it.second;
        return total;
    }

    ~Cart() {
        for (auto& it : items) delete it.first;
    }
};

// --- User ---
class User {
private:
    string name;
    Cart* cart;

public:
    User(string name) : name(name) { cart = new Cart(); }
    ~User() { delete cart; }

    Cart* getCart() { return cart; }
    string getName() { return name; }
};

// --- HttpRequest (Builder Pattern) ---
class HttpRequest {
private:
    string url;
    string method;
    map<string, string> headers;
    map<string, string> queryParams;
    string body;
    int timeout;

    HttpRequest() {}
    friend class HttpRequestBuilder;

public:
    void execute() {
        cout << "Executing " << method << " request to " << url << endl;

        if (!queryParams.empty()) {
            cout << "Query Parameters:" << endl;
            for (const auto& param : queryParams)
                cout << "  " << param.first << "=" << param.second << endl;
        }

        cout << "Headers:" << endl;
        for (const auto& header : headers)
            cout << "  " << header.first << ": " << header.second << endl;

        if (!body.empty())
            cout << "Body: " << body << endl;

        cout << "Timeout: " << timeout << " seconds" << endl;
        cout << "Request executed successfully!" << endl;
    }
};

class HttpRequestBuilder {
private:
    HttpRequest req;

public:
    HttpRequestBuilder& withUrl(const string& u) {
        req.url = u; return *this;
    }

    HttpRequestBuilder& withMethod(string method) {
        req.method = method; return *this;
    }

    HttpRequestBuilder& withHeader(const string& key, const string& value) {
        req.headers[key] = value; return *this;
    }

    HttpRequestBuilder& withQueryParams(const string& key, const string& value) {
        req.queryParams[key] = value; return *this;
    }

    HttpRequestBuilder& withBody(const string& body) {
        req.body = body; return *this;
    }

    HttpRequestBuilder& withTimeout(int timeout) {
        req.timeout = timeout; return *this;
    }

    HttpRequest build() {
        if (req.url.empty()) throw runtime_error("URL cannot be empty");
        return req;
    }
};

class HttpRequestDirector {
public:
    static HttpRequest createGetRequest(const string& url) {
        return HttpRequestBuilder().withUrl(url).withMethod("GET").build();
    }

    static HttpRequest createJsonPostRequest(const string& url, const string& jsonBody) {
        return HttpRequestBuilder()
            .withUrl(url)
            .withMethod("POST")
            .withHeader("Content-Type", "application/json")
            .withHeader("Accept", "application/json")
            .withBody(jsonBody)
            .build();
    }
};

// --- Main ---
int main() {
    InventoryManager manager(new DbInventoryStore());
    manager.addStock(101, 10);
    manager.addStock(102, 5);

    User* user = new User("Aditya");
    Cart* cart = user->getCart();
    cart->addItem(101, 2); // Apple x2
    cart->addItem(102, 3); // Banana x3

    double total = cart->getTotal();
    string jsonPayload = "{\"user\": \"" + user->getName() + "\", \"amount\": " + to_string(total) + "}";

    HttpRequest request = HttpRequestDirector::createJsonPostRequest(
        "https://zepto-lite.com/order/place",
        jsonPayload
    );

    cout << "\n--- Placing Order ---" << endl;
    request.execute();

    delete user;
    return 0;
}
