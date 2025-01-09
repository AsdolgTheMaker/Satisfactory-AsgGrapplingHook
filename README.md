Get mod from SMR: [https://ficsit.app/mod/AsgGrapplingHook](https://ficsit.app/mod/AsgGrapplingHook)

---
  
<br>

## 🪢What is this?

This mod adds a new tool - **Grappling Hook**, or Grapple for short. Grapple can be used to attach yourself with a rope to a surface, and then swing around that point. That's pretty much it. While it doesn't sound like a big change, this gives a lot of movement freedom along all three dimensions, be it on natural environment or among base buildings.

---

### 💡 How to unlock and craft it?

You need to complete a new milestone called **Grappling Hook** under **Tier 2** in **HUB**. After that, the new tool will be craftable in **Equipment Workshop** and in **Manufacturer** (under **Tools** category).

---

### ✋ How to use it?

Equip it in hands like any other tool. Cable physics are implemented in arcade way - this holds barely any realism, but designed to give movement freedom. As in, *you still have full air control while hanging on the cable, and retracting cable gives fairly large velocity change*.

* #####  **Input**

> Use **primary fire (LMB)** to shoot a hook with a cable somewhere. After it hits something, tension force will not allow you to go away from the hook, if cable is tense. You can **extend (E)** to loose the cable down, and **retract (R)** to tense it up and pull yourself closer.  
These keys can be changed in Keybindings settings (there's a new category **Grappling Hook**).

* ##### **Aim assist**

> ![Crosshair highlight appears when your aim target is within grapple's reach](https://raw.githubusercontent.com/AsdolgTheMaker/ExternalResources/refs/heads/main/Satisfactory/AsgGrapplingHook/CrosshairHighlightExample.png)  

> There is a little blue indicator around crosshair that shows up when grapple will hit something if shot right now.

* ##### **Length-o-meter**

> ![Length-o-meter displays percentage of how much your rope is used](https://raw.githubusercontent.com/AsdolgTheMaker/ExternalResources/refs/heads/main/Satisfactory/AsgGrapplingHook/LengthometerExample.png)  

> There is a little measure display at the back of the tool. It shows how much length of the rope you have already extended. Maximum rope length limit is 60m. Default length is the distance at which you was located from the hook when it hit something, then you can **retract (R)** and **extend (E)** it manually.

General advice is to retract cable if you want to build up velocity, and extend it if you want to slow down.

---


### 🧑‍🤝‍🧑🧑🏽‍🤝‍🧑🏿🧑🏼‍🤝‍🧑🏻 Multiplayer

<u>Supported, including dedicated servers!</u> However, I am generally bad at writing networking code - therefore, it works best on host / in singleplayer. Clients in multiplayer will have everything technically working, with some minor visual issues. The most noticable/annoying one is movement being slightly shaky when tension force is applied.  
*Honestly I'm surprised I managed to make this work at all.*  

Also yes, you can attach yourself to other players.

---

### ❓ F.A.Q.

* **Q:** Does this do any damage?<br>
**A:** It does, but very small amount. You *can* use it as a weapon, but a very uneffective one.

* **Q:** But does it at least let you pull towards the mobs?<br>
**A:** Yes! Grapple is implemented as a sticky projectile, so it will attach to any movable surface (but not conveyor belts).

* **Q:** When I'm on the ground, tension force is significally weaker.<br>
**A:** Yeah, partially intended. The cause for this is when you stand on the ground, you have friction applied to your movement, so any velocity change is way less significant. I left this in as another way to control the grapple. Think about it like you are resisting the force by stomping yourself into the ground. And if you want the tension force to start affecting you fully - just jump, there is no friction in the air.

* **Q:** Can I pull anything to myself with the grapple?<br>
**A:** No, tension force affects only the player who is using the tool. So items or creatures will be unaffected. But they will affect you - as in, if you shoot grapple into a creature, tension force will follow its movement. But be aware about the previous paragraph - ground friction will not allow you to just slide behind someone without jumping.

* **Q:** Cable reeling sound is weird/doesn't work properly in multiplayer/annoying, please fix.<br>
**A:** I did my best, but this came at the last moment where I had barely any energy left to keep working on this, so instead I added an option in **Mod Settings** -> **Grappling Hook** to completely disable reeling sound.

* **Q:** Is this cheating?<br>
**A:** Eh, maybe it can feel like that to someone. First of all, it's very easy to accidentally kill yourself while using it (but really, just gotta get some 10 minutes experience with it, and dying becomes a very rare occastion). To me, it's fun to explore this way, because I love swinging around on a rope in games in general. I think this love was ignited by Worms series when I was a kid. And now it reached Satisfactory.

---

### 🗈 Feedback & Reports

If there is something you want to ask or report, please reach out to **@asdolg** in [Satisfactory Modding Discord server](https://discord.gg/R85q7SrtUD).
