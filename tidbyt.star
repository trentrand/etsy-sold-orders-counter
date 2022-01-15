load("render.star", "render")
load("encoding/base64.star", "base64")
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

ETSY_ICON = base64.decode("""
iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAbhJREFUOE9VU0FOG0EQrLaD0a6N8pTkwIkD4p4Dx3wA2wIi8YycESC07J0c+QI3npE/WN61ktg7oap7lvXI8q57Zqqrq8rWXCABhqSHxZuBy7/Bnb7Ok0jGj3ZsPTf+3CvGDRjLMOIGzAA2OayJgRkO578wPv6uu7m189pn0y5G4F2BpkQAizOOWFadQDaLGMDbI02mmN6t0CxG3tknh60JMOhT1DuNIwDz+fNI0yqhXXoxhV4awXv7Kmv+SupEaK7x2RW2r4/IhSwJhbT1BftFtwSUT53AmuXIabJWJ/SzOye5wHtyIcbRRlF3YaHKPbNmMYZ9qOfuUAYycDXcyrLu9JpnxeEMxf0KG4qntilcCI/EgHjOCWW1k0ft3MPCcYqqQ7t0BhRvkK6wsU9OAIAM6LfMDnkZqITidoXNzedgwSRSRJ2TZBqBu6Qsq9TV03Xw5RyfTufY3H3ro26NRIxoTWYo71fabK+PgL/rUNEw/nqOyeUL/j3/wPb1IcsBAfD+NNTvGYvUMMge6z8/T7D7/eaCsuJBcnX7gJBwHov17KcbNVj6M/kJxTFfHMR3r+65UiM937v8B+s/6pyGqfqiAAAAAElFTkSuQmCC
""")

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
        child = render.Box(
            render.Row(
                expanded=True,
                main_align="space_evenly",
                cross_align="center",
                children = [
                    render.Image(src=ETSY_ICON),
                    render.Column(
                         children=[
                              render.Text("%d" % sales),
                              render.Text("sales"),
                         ],
                    )
                ],
            ),
        ),
    )
