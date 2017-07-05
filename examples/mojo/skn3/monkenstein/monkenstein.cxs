
Import mojo

Const TYPE_NONE := 0

Const TEXTURE_WIDTH := 64
Const TEXTURE_HEIGHT := 64
Const TEXTURE_TOTAL := 8

Const MAP_WIDTH := 24
Const MAP_HEIGHT := 16

Const SCREEN_WIDTH := 640
Const SCREEN_HEIGHT := 480

Const BACKGROUND_WIDTH := 128.0
Const BACKGROUND_HEIGHT := 128.0
Const BACKGROUND_SCALEX:Float = Float(SCREEN_WIDTH) / BACKGROUND_WIDTH
Const BACKGROUND_SCALEY:Float = Float(SCREEN_HEIGHT) / BACKGROUND_HEIGHT

Const MINIMAP_WIDTH := 6.0
Const MINIMAP_HEIGHT := 6.0
Const MINIMAP_SCALEX := MINIMAP_WIDTH / TEXTURE_WIDTH
Const MINIMAP_SCALEY := MINIMAP_HEIGHT / TEXTURE_HEIGHT

Const MINIMAP_CONTAINER_PADDING := 3
Const MINIMAP_CONTAINER_X := 16
Const MINIMAP_CONTAINER_Y := 16
Const MINIMAP_CONTAINER_WIDTH := MINIMAP_WIDTH * MAP_WIDTH
Const MINIMAP_CONTAINER_HEIGHT := MINIMAP_HEIGHT * MAP_HEIGHT

Function Main()
	New myApp
End

Class myApp Extends App
    Field posX := 21.0
    Field posY := 3.0
    Field dirX := -1.0
    Field dirY := 0.0
    Field planeX := 0.0
    Field planeY := 0.66
    Field r:Int
    Field g:Int
    Field b:Int
    Field oldDirX:Float
    Field oldDirY:Float
    Field oldPlaneX:Float
    Field oldPlaneY:Float
    Field cameraX:Float
    Field rayPosX:Float
    Field rayPosY:Float
    Field rayDirX:Float
    Field rayDirY:Float
    Field mapX:Int
    Field mapY:Int
    Field sideDistX:Float
    Field sideDistY:Float
    Field deltaDistX:Float
    Field deltaDistY:Float
    Field perpWallDist:Float
    Field stepX:Int
    Field stepY:Int
    Field hit:Bool
    Field side:Bool
    Field moveSpeed:Float
    Field rotSpeed:Float
    Field lineHeight:Float
    Field drawStart:Float
    Field drawScale:Float
    Field texNum:Int
    Field wallX:Float
    Field texX:Int
    Field frameTime:Float
    Field time:Int
    Field oldTime:Int
    Field worldMap:Int[MAP_WIDTH * MAP_HEIGHT]
    
    Field gfx:Image
    Field tiles:Image
    Field light:Image[TEXTURE_TOTAL]
    Field dark:Image[TEXTURE_TOTAL]
    Field background:Image
    
	Method OnCreate()
        'make the level data
        Local levelData := ""
        levelData += "777777777777222224444444"
        levelData += "700000000007000004000004"
        levelData += "700000000007444444000004"
        levelData += "700022120000000000000004"
        levelData += "700010020007440444444444"
        levelData += "700020010007040400000004"
        levelData += "700021220007040000000004"
        levelData += "700000000007044444444444"
        levelData += "700000000007007777777772"
        levelData += "777007777777007000000002"
        levelData += "700000007000007000000002"
        levelData += "770777777000007000055502"
        levelData += "200070007777777000055502"
        levelData += "200070000000000000055502"
        levelData += "200000007777777000000002"
        levelData += "222222222222222222222222"

        For Local index := 0 Until levelData.Length()
            worldMap[index] = Int(levelData[index..index+1])
        Next

        'build some textures
        gfx = LoadImage("textures.png")
        
        For Local index := 0 Until TEXTURE_TOTAL
            light[index] = gfx.GrabImage(index*TEXTURE_WIDTH,0,1,TEXTURE_HEIGHT,TEXTURE_WIDTH)
            dark[index] = gfx.GrabImage(index*TEXTURE_WIDTH,TEXTURE_HEIGHT,1,TEXTURE_HEIGHT,TEXTURE_WIDTH)
        Next
        
        tiles = gfx.GrabImage(0,0,TEXTURE_WIDTH,TEXTURE_HEIGHT,TEXTURE_TOTAL)

        background = gfx.GrabImage(0,128,BACKGROUND_WIDTH,BACKGROUND_HEIGHT)

        SetUpdateRate(30)
    End
    
	Method OnRender()
        DrawImage(background,0,0, 0,BACKGROUND_SCALEX,BACKGROUND_SCALEY)
        
        For Local x := 0 Until SCREEN_WIDTH
            cameraX = (2*x)/Float(SCREEN_WIDTH)-1
            rayPosX = posX
            rayPosY = posY
            rayDirX = dirX + planeX*cameraX
            rayDirY = dirY + planeY*cameraX
            mapX = Int(rayPosX)
            mapY = Int(rayPosY)
            deltaDistX = Sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
            deltaDistY = Sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
            hit = False
            side = False

            'calculate step and initial sideDist
            If rayDirX < 0
                stepX = -1
                sideDistX = (rayPosX - mapX) * deltaDistX
            Else
                stepX = 1
                sideDistX = (mapX + 1.0 - rayPosX) * deltaDistX
            Endif

            If rayDirY < 0
                stepY = -1
                sideDistY = (rayPosY - mapY) * deltaDistY
            Else
                stepY = 1
                sideDistY = (mapY + 1.0 - rayPosY) * deltaDistY
            Endif

            'perform DDA
            While hit = False
                'jump to next map square, OR in x-direction, OR in y-direction
                If sideDistX < sideDistY
                    sideDistX = sideDistX + deltaDistX
                    mapX = mapX + stepX
                    side = False
                Else
                    sideDistY = sideDistY + deltaDistY
                    mapY = mapY + stepY
                    side = True
                Endif

                'Check If ray has hit a wall
                If worldMap[WorldIndex(mapX,mapY)] > 0
                    hit = True
                Endif
            Wend

            'Calculate distance of perpendicular ray (oblique distance will give fisheye effect!)
            If side = False
                perpWallDist = Abs((mapX - rayPosX + (1 - stepX) / 2) / rayDirX)
            Else
                perpWallDist = Abs((mapY - rayPosY + (1 - stepY) / 2) / rayDirY)
            Endif

            lineHeight = Abs(Int(SCREEN_HEIGHT / perpWallDist))
            drawStart = (-lineHeight / 2) + (SCREEN_HEIGHT / 2)
            drawScale = Float(lineHeight) / 64.0
            texNum = worldMap[WorldIndex(mapX,mapY)]-1

            If texNum > -1 And drawScale > 0
                'calculate value of wallX
                If side = True
                    wallX = rayPosX + ((mapY - rayPosY + (1 - stepY) / 2) / rayDirY) * rayDirX
                Else
                    wallX = rayPosY + ((mapX - rayPosX + (1 - stepX) / 2) / rayDirX) * rayDirY
                Endif
                wallX -= Floor(wallX)

                'x coordinate on the texture
                texX = Int(wallX * Float(TEXTURE_WIDTH))
                If side = False And rayDirX > 0
                    texX = TEXTURE_WIDTH - texX - 1
                Endif
                If side = True And rayDirY < 0
                    texX = TEXTURE_WIDTH - texX - 1
                Endif

                If side = True
                    DrawImage(dark[texNum],x,drawStart,0,1,drawScale,texX)
                Else
                    DrawImage(light[texNum],x,drawStart,0,1,drawScale,texX)
                Endif
            Endif
        Next
        
        'render mini map
        Translate(MINIMAP_CONTAINER_X,MINIMAP_CONTAINER_Y)
        SetColor(255,255,255)
        DrawRect(0,0,MINIMAP_CONTAINER_WIDTH+MINIMAP_CONTAINER_PADDING+MINIMAP_CONTAINER_PADDING,MINIMAP_CONTAINER_HEIGHT+MINIMAP_CONTAINER_PADDING+MINIMAP_CONTAINER_PADDING)
        
        Translate(MINIMAP_CONTAINER_PADDING,MINIMAP_CONTAINER_PADDING)
        For Local y := 0 Until MAP_HEIGHT
            For Local x := 0 Until MAP_WIDTH
                texNum = worldMap[WorldIndex(x,y)]-1
                
                If texNum > -1
                    DrawImage(tiles,x*MINIMAP_WIDTH,y*MINIMAP_HEIGHT, 0, MINIMAP_SCALEX,MINIMAP_SCALEY, texNum)
                Endif
            Next
        Next
        
        'render player on minimap
        SetColor(255,0,0)
        DrawRect(posX*MINIMAP_WIDTH-(MINIMAP_WIDTH/2),posY*MINIMAP_HEIGHT-(MINIMAP_HEIGHT/2),MINIMAP_WIDTH,MINIMAP_HEIGHT)
    End

    Method OnUpdate()
        oldTime = time
        time = Millisecs()
        frameTime = Float(time - oldTime) / 1000.0
        moveSpeed = frameTime * 4.0
        rotSpeed = frameTime * 90.0

        'move forward If no wall in front of you
        If KeyDown(KEY_UP)
            If worldMap[WorldIndex(posX + (dirX * moveSpeed),posY)] = False
                posX = posX + dirX * moveSpeed
            Endif
            If worldMap[WorldIndex(posX,posY + (dirY * moveSpeed))] = False
                posY = posY + dirY * moveSpeed
            Endif
        Endif


        'move backwards If no wall behind you
        If KeyDown(KEY_DOWN)
            If worldMap[WorldIndex(posX - (dirX * moveSpeed),posY)] = False
                posX = posX - dirX * moveSpeed
            Endif
            If worldMap[WorldIndex(posX,posY - (dirY * moveSpeed))] = False
                posY = posY - dirY * moveSpeed
            Endif
        Endif

        'rotate to the right
        If KeyDown(KEY_RIGHT)
            'both camera direction and camera plane must be rotated
            oldDirX = dirX
            dirX = dirX * Cos(-rotSpeed) - dirY * Sin(-rotSpeed)
            dirY = oldDirX * Sin(-rotSpeed) + dirY * Cos(-rotSpeed)
            
            oldPlaneX = planeX
            planeX = planeX * Cos(-rotSpeed) - planeY * Sin(-rotSpeed)
            planeY = oldPlaneX * Sin(-rotSpeed) + planeY * Cos(-rotSpeed)
        Endif

        'rotate to the left
        If KeyDown(KEY_LEFT)
            'both camera direction and camera plane must be rotated
            oldDirX = dirX
            dirX = dirX * Cos(rotSpeed) - dirY * Sin(rotSpeed)
            dirY = oldDirX * Sin(rotSpeed) + dirY * Cos(rotSpeed)
            oldPlaneX = planeX
            planeX = planeX * Cos(rotSpeed) - planeY * Sin(rotSpeed)
            planeY = oldPlaneX * Sin(rotSpeed) + planeY * Cos(rotSpeed)
        Endif
    End

	Method Set(x:Int,y:Int,type:Int=TYPE_NONE)
        level[WorldIndex(x,y)] = type
	End

	Method WorldIndex:Int(x:Int,y:Int)
        Return (MAP_WIDTH * y)+x
	End
End
