load("render.star", "render")
load("encoding/base64.star", "base64")
load("http.star", "http")
load("cache.star", "cache")
load("schema.star", "schema")

ETSY_ICON = base64.decode("""
iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAAAXNSR0IArs4c6QAAAbhJREFUOE9VU0FOG0EQrLaD0a6N8pTkwIkD4p4Dx3wA2wIi8YycESC07J0c+QI3npE/WN61ktg7oap7lvXI8q57Zqqrq8rWXCABhqSHxZuBy7/Bnb7Ok0jGj3ZsPTf+3CvGDRjLMOIGzAA2OayJgRkO578wPv6uu7m189pn0y5G4F2BpkQAizOOWFadQDaLGMDbI02mmN6t0CxG3tknh60JMOhT1DuNIwDz+fNI0yqhXXoxhV4awXv7Kmv+SupEaK7x2RW2r4/IhSwJhbT1BftFtwSUT53AmuXIabJWJ/SzOye5wHtyIcbRRlF3YaHKPbNmMYZ9qOfuUAYycDXcyrLu9JpnxeEMxf0KG4qntilcCI/EgHjOCWW1k0ft3MPCcYqqQ7t0BhRvkK6wsU9OAIAM6LfMDnkZqITidoXNzedgwSRSRJ2TZBqBu6Qsq9TV03Xw5RyfTufY3H3ro26NRIxoTWYo71fabK+PgL/rUNEw/nqOyeUL/j3/wPb1IcsBAfD+NNTvGYvUMMge6z8/T7D7/eaCsuJBcnX7gJBwHov17KcbNVj6M/kJxTFfHMR3r+65UiM937v8B+s/6pyGqfqiAAAAAElFTkSuQmCC
""")

def main(config):
    if config.get("user_id_or_name") == None or config.get("api_key") == None:
      return render.Root(
        child =
          render.Column(
               children=[
                    render.Text("Etsy Sold"),
                    render.Text("Orders app"),
                    render.Text("requires"),
                    render.Text("configuration"),
               ],
          )
      )

    sales_cached = cache.get("sales")
    if sales_cached != None:
        print("Hit! Displaying cached data.")
        sales = int(sales_cached)
    else:
        print("Miss! Calling Etsy API.")
        request_url = "https://openapi.etsy.com/v2/users/{user_id_or_name}/profile?api_key={api_key}".format(
          user_id_or_name=config.str("user_id_or_name"),
          api_key=config.str("api_key"),
        )

        response = http.get(request_url)
        """
        {
         ...,
         status_code: 200,
         results: [{
           ...,
           transaction_sold_count: 123
         }]
        }
        """
        if response.status_code != 200:
            fail("Etsy request failed with status %d", response.status_code)
        sales = response.json()["results"][0]["transaction_sold_count"]
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

"""TODO: use oauth, not raw key entry"""
def get_schema():
    return schema.Schema(
        version = "1",
        fields = [
          schema.Text(
            id = "user_id_or_name",
            name = "Etsy user id or name",
            desc = "Specify user's numeric ID or login name.",
            icon = "user",
          ),
          schema.Text(
            id = "api_key",
            name = "Etsy API key",
            desc = "Specify user created Etsy api key",
            icon = "signatureLock",
          ),
        ],
    )
