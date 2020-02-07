/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

namespace KODI
{
namespace RETRO
{
  class CAction;
  class CPolicy;
  class CReward;
  class CState;

  /*!
   * \brief Game-Theoretic Emulation Engine
   *
   * ===========================================================================
   * 1. Introduction
   *
   * Here, the Emulation Equation is introduced, a game-theoretic equation for
   * emulation.
   *
   * The equation represents all emulation, allows for powerful features, and
   * all gameplay (human or otherwise) becomes data for training an
   * artificially-intelligent game-playing agent.
   *
   * For demonstration purposes, the explanation is split into two theories:
   *
   *   - The Special Theory of Emulation explains the fundamentals, and
   *     presents a simplified emulation equation. Using this, all emulation
   *     and many powerful features are possible.
   *
   *   - The General Theory of Emulation uses the same fundamentals, and
   *     presents an extended emulation equation. Using this, all gameplay
   *     (human or otherwise) becomes data for training a reinforcement
   *     learner.
   *
   * For the physics nerds, this is analogous to how Einstein introduced
   * relativity:
   *
   *   - In 1905, his Special Theory of Relativity explained moving bodies
   *     without gravity
   *
   *   - In 1915, his General Theory of Relativity explained moving bodies
   *     in the presence of gravity
   *
   * ===========================================================================
   * 2. Background
   *
   * Reinforcement learning is popular in game-playing AI because the reward
   * signal is often sparse (not evident every frame) and depends on actions
   * taken much earlier in the game.
   *
   * The Q-learning algorithm is well-suited for teaching reinforcement
   * learners how to play a video game because it does not require a model of
   * the environment, which would be difficult to create for even the most
   * basic computing machine.
   *
   * Recall the Q-learning algorithm (see https://en.wikipedia.org/wiki/Q-learning):
   *
   *   Q_new(S_t, A_t) <- Q(S_t, A_t) + alpha + [R_t + gamma * max/a Q(S_t+1, a) - Q(S_t, A_t)]
   *
   * Understanding what the f*ck that means is out of the scope of this
   * documentation. Just know that it learns over "emulation equations",
   * which describe a series of frames in the discrete time domain.
   *
   * Emulation equations are described here for two theories of emulation, the
   * Special Theory of Emulation and the General Theory of Emulation.
   *
   * ===========================================================================
   * 3. The Special Theory of Emulation
   *
   * This theory models emulation using a simpler equation not suited for
   * Q-learning.
   *
   * ---------------------------------------------------------------------------
   * 3.1. Emulation variables
   *
   * The environment contains two variables:
   *
   *   State === S
   *
   * The state consists of video, audio, and memory regions (RAM, SRAM,
   * Real-time clock).
   *
   *   Action === A
   *
   * The action is the combined state of all input devices.
   *
   * ---------------------------------------------------------------------------
   * 3.2. Time series
   *
   * Emulation occurs at discrete time steps, so every time step has its
   * instance of these variables:
   *
   *   S === S_t
   *   A === A_t
   *
   * The emulation history is therefore a time series of tuples:
   *
   *   (S_0, A_0, S_1, A_1, ...)
   *
   * ---------------------------------------------------------------------------
   * 3.3. Emulation model
   *
   * The time series is modeled by two functions:
   *
   *   PlayFrame()
   *
   * The PlayFrame() function takes the previous state, along with the most
   * recent action, and produces a new state.
   *
   *   GetInput()
   *
   * The GetInput() function takes the previous action, along with the most
   * recent state, and produces a new action.
   *
   * ---------------------------------------------------------------------------
   * 3.4. Emulation equation
   *
   * The emulation equation is a time series model consisting of the initial
   * conditions, as well as the model used for each time step.
   *
   * The initial condition of all variables is the empty set:
   *
   *   S_0 === 0
   *   A_0 === 0
   *
   * The emulation model is:
   *
   *   S_t+1 = PlayFrame( S_t,   A_t )
   *   A_t+1 = GetInput(  S_t+1, A_t )
   *
   * ---------------------------------------------------------------------------
   * 3.5. Summary
   *
   * The emulation equation describes something fundamental in every emulator;
   * play a frame, get input, repeat.
   *
   * Interestingly, this fundamental concept was not a priori. It emerged as a
   * model from solving an algorithm.
   *
   * Next, we present the General Theory of Emulation, which expands on these
   * fundamentals to include two new concepts needed in the Q-learning
   * algorithm.
   *
   * ===========================================================================
   * 4. The General Theory of Emulation
   *
   * This theory extends the basic emulation equation so that it can be used
   * for Q-learning.
   *
   * ---------------------------------------------------------------------------
   * 4.1. Emulation variables
   *
   * Reinforcement learning needs two additional environment variables:
   *
   *   Reward === R
   *
   * The reward is used to train the function approximator that infers actions
   * from the observed state. The reward can come from sniffing values in RAM,
   * or extracting numbers from video memory using OCR.
   *
   *   Policy === P
   *
   * The policy is used by the agent to choose the next move. The goal of
   * reinforcement learning is to choose the policy that maximizes the reward.
   *
   * ---------------------------------------------------------------------------
   * 4.2. Time series
   *
   * Emulation occurs at discrete time steps, so every time step has its
   * instance of the environment's variables:
   *
   *   S === S_t
   *   R === R_t
   *   P === P_t
   *   A === A_t
   *
   * The emulation history is therefore a time series of tuples:
   *
   *   (S_0, R_0, P_0, A_0, S_1, R_1, P_1, A_1, ...)
   *
   * ---------------------------------------------------------------------------
   * 4.3. Emulation model
   *
   * Reinforcement learning also needs two more functions:
   *
   *   GetReward()
   *
   * The GetReward() function takes the previous reward, along with the most
   * recent values of the other variables, and produces a new reward.
   *
   *   Strategize()
   *
   * The Strategize() function takes the previous policy, along with the most
   * recent values of the other variables, and produces a new policy.
   *
   * ---------------------------------------------------------------------------
   * 4.4. Emulation equation
   *
   * The emulation equation is a time series model consisting of the initial
   * conditions, as well as the model used for each time step.
   *
   * The initial condition of all variables is the empty set:
   *
   *   S_0 === 0
   *   R_0 === 0
   *   P_0 === 0
   *   A_0 === 0
   *
   * The emulation model is:
   *
   *   S_t+1 = PlayFrame(  S_t,   R_t,   P_t,   A_t )
   *   R_t+1 = GetReward(  S_t+1, R_t,   P_t,   A_t )
   *   P_t+1 = Strategize( S_t+1, R_t+1, P_t,   A_t )
   *   A_t+1 = GetInput(   S_t+1, R_t+1, P_t+1, A_t )
   */
  class IEmulationEngine
  {
  public:
    virtual ~IEmulationEngine() = default;

    using T = uint64_t;
    using S = CState;
    using R = CReward;
    using P = CPolicy;
    using A = CAction;

    // Environment functions
    // clang-format off
    virtual void PlayFrame(  T t,       S& state, const R& reward, const P& policy, const A& action ) = 0;
    virtual void GetReward(  T t, const S& state,       R& reward, const P& policy, const A& action ) = 0;
    virtual void Strategize( T t, const S& state, const R& reward,       P& policy, const A& action ) = 0;
    virtual void GetInput(   T t, const S& state, const R& reward, const P& policy,       A& action ) = 0;
    // clang-format on
  };
}
}
