![portkey_led_board](https://github.com/user-attachments/assets/b6a7d41f-6fcf-4bad-a230-3603f8bf1db8)

This is a P10 LED board setup I put up in the portkey office that shows a scrolling ticker board style metrics of the live usage on portkey

we process around 18M requests a day(that is a 13000+ Requests Per Minute!)

I published the dashboard at [https://portkey.ai/rankings/daily](https://portkey.ai/rankings/daily) (this is server side rendered and refreshed hourly)


## Deetails
- I've added a claude.md file (claude really helped with modifying the code for the generic Chinese P10 boards that the guy who scaffoled the boards sold me :' )
- Basically p10 boards connected sequentially but run text parallelly
- controlled with an ESP32
- I did not solder or put glue on the wiring lol, I know this might break at some point, I just plugged in the jumper cables and shipped it
