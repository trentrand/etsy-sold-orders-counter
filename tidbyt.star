load("render.star", "render")
load("http.star", "http")
load("cache.star", "cache")

ETSY_SALES_URL = "https://openapi.etsy.com/v2/users/your_user_id_or_name/profile?api_key=your_api_key"
# Response payload:
# {
#   ...,
#   results: [{
#     ...,
#     transaction_sold_count: 1234
#   }]
#  }


def main():
    sales_cached = cache.get("sales")
    if sales_cached != None:
        print("Hit! Displaying cached data.")
        sales = int(sales_cached)
    else:
        print("Miss! Calling Etsy API.")
        rep = http.get(ETSY_SALES_URL)
        if rep.status_code != 200:
            fail("Etsy request failed with status %d", rep.status_code)
        sales = rep.json()["results"][0]["transaction_sold_count"]
        cache.set("sales", str(int(sales)), ttl_seconds=240)

    return render.Root(
        child = render.Text("Sales: %d " % sales)
    )
