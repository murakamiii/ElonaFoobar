ELONA.i18n:add {
   food = {
      -- Names for cooked food.
      -- These are organized by type, then quality.
      names = {
         -- Meat
         _1 = {
            default_origin = "動物",
            _1 = "グロテスクな{$1}の肉",
            _2 = "焼け焦げた{$1}の肉",
            _3 = "{$1}のこんがり肉",
            _4 = "{$1}肉のオードブル",
            _5 = "{$1}のピリ辛炒め",
            _6 = "{$1}コロッケ",
            _7 = "{$1}のハンバーグ",
            _8 = "{$1}肉の大葉焼き",
            _9 = "{$1}ステーキ",
         },
         -- Vegetable
         _2 = {
            default_origin = "野菜",
            _1 = "生ごみ同然の{$1}",
            _2 = "悪臭を放つ{$1}",
            _3 = "{$1}のサラダ",
            _4 = "{$1}の炒め物",
            _5 = "{$1}風味の肉じゃが",
            _6 = "{$1}の天ぷら",
            _7 = "{$1}の煮込み",
            _8 = "{$1}シチュー",
            _9 = "{$1}風カレー",
         },
         -- Fruit
         _3 = {
            default_origin = "果物",
            _1 = "食べてはならない{$1}",
            _2 = "べっちょりした{$1}",
            _3 = "{$1}のフルーツサラダ",
            _4 = "{$1}のプリン",
            _5 = "{$1}シャーベット",
            _6 = "{$1}シェイク",
            _7 = "{$1}クレープ",
            _8 = "{$1}フルーツケーキ",
            _9 = "{$1}パフェ",
         },
         -- Candy
         _4 = {
            default_origin = "お菓子",
            _1 = "原型を留めない{$1}",
            _2 = "まずそうな{$1}",
            _3 = "{$1}クッキー",
            _4 = "{$1}のゼリー",
            _5 = "{$1}パイ",
            _6 = "{$1}まんじゅう",
            _7 = "{$1}風味のシュークリーム",
            _8 = "{$1}のケーキ",
            _9 = "{$1}風ザッハトルテ",
         },
         -- Noodle
         _5 = {
            default_origin = "麺",
            _1 = "禁断の{$1}",
            _2 = "のびてふにゃった{$1}",
            _3 = "サラダパスタ",
            _4 = "うどん",
            _5 = "冷やし蕎麦",
            _6 = "ペペロンチーノ",
            _7 = "カルボナーラ",
            _8 = "ラーメン",
            _9 = "ミートスパゲティ",
         },
         -- Fish
         _6 = {
            default_origin = "魚",
            _1 = "{$1}の残骸",
            _2 = "骨だけ残った{$1}",
            _3 = "{$1}のフライ",
            _4 = "{$1}の煮込み",
            _5 = "{$1}スープ",
            _6 = "{$1}の天ぷら",
            _7 = "{$1}ソーセージ",
            _8 = "{$1}の刺身",
            _9 = "{$1}の活け作り",
         },
         -- Bread
         _7 = {
            default_origin = "パン",
            _1 = "恐怖の{$1}",
            _2 = "ガチガチの{$1}",
            _3 = "くるみパン",
            _4 = "アップルパイ",
            _5 = "サンドイッチ",
            _6 = "クロワッサン",
            _7 = "コロッケパン",
            _8 = "カレーパン",
            _9 = "メロンパン",
         },
         -- Egg
         _8 = {
            default_origin = "鳥",
            _1 = "グロテスクな{$1}の卵",
            _2 = "焦げた{$1}の卵",
            _3 = "{$1}の卵の目玉焼き",
            _4 = "{$1}風味のキッシュ",
            _5 = "半熟{$1}",
            _6 = "{$1}の卵入りスープ",
            _7 = "熟成{$1}チーズ",
            _8 = "{$1}のレアチーズケーキ",
            _9 = "{$1}風オムライス",
         },
      },

      passed_rotten = {
         "「うぐぐ！なんだこの飯は！」",
         "「うっ！」",
         "「……！！」",
         "「あれれ…」",
         "「…これは何の嫌がらせですか」",
         "「まずい！」",
      },

      mochi = {
         chokes = "{name($1)}はもちを喉につまらせた！",
         dialog = "「むがっ」",
      },

      hunger_status = {
         hungry = {
            "腹がすいてきた。",
            "空腹になった。",
            "さて何を食べようか。",
         },
         very_hungry = {
            "空腹で目が回りだした…",
            "すぐに何かを食べなくては…",
         },
         starving = {
            "このままだと餓死してしまう！",
            "腹が減ってほとんど死にかけている。",
         },
      },

      eating_message = {
         bloated = {
            "もう当分食べなくてもいい。",
            "こんなに食べたことはない！",
            "信じられないぐらい満腹だ！",
         },
         satisfied = {
            "あなたは満足した。",
            "満腹だ！",
            "あなたは食欲を満たした。",
            "あなたは幸せそうに腹をさすった。",
         },
         normal = {
            "まだ食べられるな…",
            "あなたは腹をさすった。",
            "少し食欲を満たした。",
         },
         hungry = {
            "まだまだ食べたりない。",
            "物足りない…",
            "まだ空腹だ。",
            "少しは腹の足しになったか…",
         },
         very_hungry = {
            "全然食べたりない！",
            "腹の足しにもならない。",
            "すぐにまた腹が鳴った。",
         },
         starving = {
            "こんな量では意味がない！",
            "これぐらいでは、死を少し先に延ばしただけだ。",
            "無意味だ…もっと栄養をとらなければ。",
         },
      },

      not_affected_by_rotten = "しかし、{name($1)}は何ともなかった。",

      anorexia = {
         develops = "{name($1)}は拒食症になった。",
         recovers_from = "{name($1)}の拒食症は治った。",
      },

      vomits = "{name($1)}は吐いた。",
      spits_alien_children = "{name($1)}は体内のエイリアンを吐き出した！",

      eat_status = {
         good = "{name($1)}は良い予感がした。",
         bad = "{name($1)}は嫌な感じがした。",
         cursed_drink = "{name($1)}は気分が悪くなった。",
      },

      cook = "{itemname($2, 1)}で{$1}を料理して、{itemname($3, 1)}を作った。",

      effect = {
         rotten = "うげっ！腐ったものを食べてしまった…うわ…",

         raw_meat = "生肉だ…",
         powder = "粉の味がする…",
         raw = "生で食べるものじゃないな…",
         boring = {
            "まずいわけではないが…",
            "平凡な味だ。",
         },

         raw_glum = "{name($1)}は渋い顔をした。",

         herb = {
            curaria = "このハーブは活力の源だ。",
            morgia = "新たな力が湧きあがってくる。",
            mareilon = "魔力の向上を感じる。",
            spenseweed = "感覚が研ぎ澄まされるようだ。",
            alraunia = "ホルモンが活発化した。",
         },

         human = {
            like = "これはあなたの大好きな人肉だ！",
            dislike = "これは人肉だ…うぇぇ！",
            would_have_rather_eaten = "人肉の方が好みだが…",
            delicious = "ウマイ！",
         },

         bomb_fish = "「げふぅ」{name($1)}は{itemname($2, 1)}を吐き出した。",
         little_sister = "{name($1)}は進化した。",
         hero_cheese = "これは縁起がいい！",

         fortune_cookie = "{name($1)}はクッキーの中のおみくじを読んだ。",
         sisters_love_fueled_lunch = "{name($1)}の心はすこし癒された。",

         poisoned = {
            text = "これは毒されている！{name($1)}はもがき苦しみのたうちまわった！",
            dialog = {
               "「ギャァァ…！」",
               "「ブッ！」",
            },
         },

         spiked = {
            self = "あなたは興奮した！",
            other = {
               "{name($1)}「なんだか…変な気分なの…」",
               "{name($1)}「あれ…なにこの感じは…」",
            },
         },

         ability = {
            develops = "{name($1)}の{$2}は発達した。",
            deteriorates = "{name($1)}の{$2}は衰えた。",
         },

         corpse = {
            iron = "まるで鉄のように硬い！{name($1)}の胃は悲鳴をあげた。",
            deformed_eye = "気が変になりそうな味だ。",
            horse = "馬肉だ！これは精がつきそうだ。",
            holy_one = "{name($1)}は神聖なものを汚した気がした。",
            at = "＠を食べるなんて…",
            guard = "ガード達はあなたを憎悪した。",
            vesda = "{name($1)}の体は一瞬燃え上がった。",
            insanity = "{name($1)}の胃は狂気で満たされた。",
            putit = "肌がつるつるになりそうだ。",
            cupid_of_love = "{name($1)}は恋をしている気分になった！",
            poisonous = "これは有毒だ！",
            troll = "血が沸き立つようだ。",
            rotten_one = "腐ってるなんて分かりきっていたのに…うげぇ",
            beetle = "力が湧いてくるようだ。",
            mandrake = "微かな魔力の刺激を感じた。",
            grudge = "胃の調子がおかしい…",
            calm = "この肉は心を落ち着かせる効果があるようだ。",
            chaos_cloud = "{name($1)}の胃は混沌で満たされた。",
            lightning = "{name($1)}の神経に電流が走った。",
            cat = "猫を食べるなんて！！",
            ether = "{name($1)}の体内はエーテルで満たされた。",
            giant = "体力がつきそうだ。",
            imp = "魔力が鍛えられる。",
            strength = "力がつきそうだ。",
            ghost = "精神が少しずぶとくなった。",
            quickling = "ワアーォ、{name($1)}は速くなった気がする！",
            alien = "何かが{name($1)}の体内に入り込んだ。",
         },

         quality = {
            bad = {
               "うぅ…腹を壊しそうだ。",
               "まずい！",
               "ひどい味だ！",
            },
            so_so = {
               "まあまあの味だ。",
               "悪くない味だ。",
            },
            good = {
               "かなりいける。",
               "それなりに美味しかった。",
            },
            great = {
               "美味しい！",
               "これはいける！",
               "いい味だ！",
            },
            delicious = {
               "最高に美味しい！",
               "まさに絶品だ！",
               "天にも昇る味だ！",
            },
         },
      },
   },
}